#!/usr/bin/env python3
"""Build a WASI Preview 3 memory32 boundary around a real memory64 core.

The engine uses its normal 64-bit libc ABI. This module copies only WASI
values through an independent memory32 heap, which existing component hosts
can lift and lower. Audio buffers never pass through this adapter.
"""
import argparse
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile

from wasm import Module, Reader, leb, add_metadata, custom, quote


def tool(*args):
    return subprocess.check_output([os.environ.get('WASM_TOOLS', 'wasm-tools'), *map(str, args)])


def signature(params, results):
    return ('(param ' + ' '.join(params) + ')' if params else '') + ('(result ' + ' '.join(results) + ')' if results else '')


def command_export(engine):
    names = [name for name, (kind, _) in engine.exports.items()
             if kind == 0 and re.fullmatch(r'wasi:cli/run@0\.3\.\d+#run', name)]
    if len(names) != 1:
        raise ValueError('Expected one WASI Preview 3 command export')
    return names[0]


def generate(engine, wit, browser):
    from preview3 import generate as generate_preview3
    if not any('@0.3.' in module for module, _, _, _ in engine.imports):
        raise ValueError('Expected a WASI Preview 3 core module')
    return generate_preview3(engine, wit, browser)


def build(core, output, browser=False):
    data = Module(Path(core).read_bytes()).import_memory()
    engine = Module(data)
    wit = json.loads(tool('component', 'wit', core, '--json'))
    source, names = generate(engine, wit, browser)
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    core_path = output.with_suffix('.core.wasm')
    bridge_path = output.with_suffix('.bridge.wasm')
    with tempfile.TemporaryDirectory() as temp:
        temp = Path(temp)
        wat = temp / 'bridge.wat'
        wat.write_text(source)
        raw = tool('parse', wat)
        bridge_path.write_bytes(raw if browser else add_metadata(raw, engine.metadata))
        tool('validate', bridge_path)
        # Bundle the core adapter in a custom section so JS loads one engine
        # artifact. Commands keep this same derivative for score preprocessing.
        if browser:
            browser_bridge = raw
        else:
            browser_source, _ = generate(engine, wit, True)
            wat.write_text(browser_source)
            browser_bridge = tool('parse', wat)
        core_data = custom(data, 'csound.abi', json.dumps(engine.abi()).encode())
        core_data = custom(core_data, 'csound.wasi-bridge', browser_bridge)
        (output if browser else core_path).write_bytes(core_data)
        if browser:
            return
        encoded = temp / 'bridge.component.wasm'
        tool('component', 'new', bridge_path, '-o', encoded)
        component = tool('print', encoded).decode().rstrip()
        assert component.endswith(')') and '(core instance $main ' in component
        # Add the memory64 engine after wit-component has connected the
        # ordinary Preview 3 imports of the bridge. No canonical operation
        # ever refers to the engine's 64-bit memory.
        # Encode a tiny stand-in to let the WAT parser resolve symbolic core
        # instance names, then replace its core-module section with LLVM's
        # binary. This avoids expanding every engine instruction into text.
        flags, minimum, maximum = engine.memories[0]
        limits = f'i64 {minimum}' + (f' {maximum}' if maximum is not None else '')
        stub_imports = [f'(import "env" "memory" (memory {limits}))']
        for module, name, kind, index in engine.imports:
            if kind == 0:
                stub_imports.append(f'(import {quote(module)} {quote(name)} (func {signature(*engine.types[index])}))')
        run_name = quote(command_export(engine))
        extra_exports = '(func (export "free") (param i64)) (func (export "__wasm_task_hook") (param i32))'
        definitions = ['(core module $engine64 ' + '\n'.join(stub_imports) + f'''
          (func (export "cabi_realloc") (param i64 i64 i64 i64) (result i64) i64.const 0)
          (func (export {run_name}) (result i32) i32.const 0) {extra_exports})''']
        definitions.append('(alias core export $main "heap" (core memory $engine-heap))')
        definitions.append('(alias core export $main "engine-functions" (core table $engine-functions))')
        groups = {}
        for index, (module, name, exported) in enumerate(names):
            definitions.append(f'(alias core export $main {quote(exported)} (core func $engine-import{index}))')
            groups.setdefault(module, []).append(f'(export {quote(name)} (func $engine-import{index}))')
        groups.setdefault('env', []).append('(export "memory" (memory $engine-heap))')
        arguments = []
        arguments += [f'(with {quote(module)} (instance ' + ' '.join(exports) + '))' for module, exports in groups.items()]
        definitions.append('(core instance $engine64 (instantiate $engine64 ' + ' '.join(arguments) + '))')
        extra_imports = '(import "engine" "free" (func $free (param i64))) (import "engine" "__wasm_task_hook" (func $hook (param i32)))'
        extra_elements = '$free $hook'
        definitions.append(f'''(core module $connect-engine
          (import "bridge" "table" (table 4 funcref))
          (import "engine" "cabi_realloc" (func $alloc (param i64 i64 i64 i64) (result i64)))
          (import "engine" {run_name} (func $run (result i32)))
          {extra_imports}
          (elem (i32.const 0) func $alloc $run {extra_elements}))
          (core instance (instantiate $connect-engine
            (with "bridge" (instance (export "table" (table $engine-functions))))
            (with "engine" (instance $engine64))))''')
        wat.write_text(component[:-1] + '\n' + '\n'.join(definitions) + '\n)')
        assembled = tool('parse', wat)
        reader = Reader(assembled)
        header = reader.take(8)
        sections = []
        while reader.pos < len(assembled):
            sections.append((reader.byte(), reader.take(reader.uint())))
        engine_section = [i for i, (kind, _) in enumerate(sections) if kind == 1][-2]
        sections[engine_section] = 1, data
        output.write_bytes(header + b''.join(bytes([kind]) + leb(len(payload)) + payload for kind, payload in sections))
        tool('validate', output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('core')
    parser.add_argument('output')
    parser.add_argument('--browser', action='store_true')
    args = parser.parse_args()
    build(args.core, args.output, args.browser)
