#!/usr/bin/env python3
"""Build a WASI Preview 2 memory32 boundary around a real memory64 core.

The engine uses its normal 64-bit libc ABI. This module copies only WASI
values through an independent memory32 heap, which existing component hosts
can lift and lower. Audio buffers never pass through this adapter.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

from abi import ABI
from wasm import Module, Reader, leb, add_metadata, custom, quote


def tool(*args):
    return subprocess.check_output([os.environ.get('WASM_TOOLS', 'wasm-tools'), *map(str, args)])


def signature(params, results):
    return ('(param ' + ' '.join(params) + ')' if params else '') + ('(result ' + ' '.join(results) + ')' if results else '')


def generate(engine, wit, browser):
    abi = ABI(wit)
    interfaces = {}
    for interface in wit['interfaces']:
        package = wit['packages'][interface['package']]['name']
        package, _, version = package.partition('@')
        module = package + '/' + interface['name'] + ('@' + version if version else '')
        interfaces[module] = interface
    imports, wrappers, names = [], [], []
    for module, name, kind, type_index in engine.imports:
        if kind != 0:
            continue
        if not module.startswith('wasi:'):
            if browser:
                continue
            raise ValueError(f'command has non-WASI import: {module}/{name}')
        args64, returns64 = engine.types[type_index]
        index = len(names)
        exported = f'{module}#{name}'
        names.append((module, name, exported))
        if name.startswith('[resource-drop]'):
            assert (args64, returns64) == (['i32'], [])
            imports.append(f'(import {quote(module)} {quote(name)} (func $host{index} (param i32)))')
            wrappers.append(f'(func (export {quote(exported)}) (param i32) (call $host{index} (local.get 0)))')
            continue
        function = interfaces[module]['functions'][name]
        parameters = abi.tuple([p['type'] for p in function['params']])
        result = function.get('result')
        flat64, flat32 = abi.flat(parameters, 64), abi.flat(parameters, 32)
        result64, result32 = abi.flat(result, 64), abi.flat(result, 32)
        indirect = len(flat64) > 16
        assert indirect == (len(flat32) > 16)
        retptr = len(result64) > 1
        assert retptr == (len(result32) > 1)
        expected64 = (['i64'] if indirect else list(flat64)) + (['i64'] if retptr else [])
        expected_result = [] if retptr else list(result64)
        assert (args64, returns64) == (expected64, expected_result), (module, name, (args64, returns64), (expected64, expected_result))
        args32 = (['i32'] if indirect else list(flat32)) + (['i32'] if retptr else [])
        returns32 = [] if retptr else list(result32)
        imports.append(f'(import {quote(module)} {quote(name)} (func $host{index} {signature(args32, returns32)}))')
        arguments = ' '.join(f'(param $a{i} {ty})' for i, ty in enumerate(args64))
        locals_ = '(local $saved i32) (local $p64 i64) (local $p32 i32) (local $r32 i32)'
        if returns64:
            locals_ += f'(local $value {returns64[0]})'
        code = ['(local.set $saved (global.get $next))']
        if flat64:
            size64, align64 = abi.layout(parameters, 64)
            if indirect:
                code.append('(local.set $p64 (local.get $a0))')
            else:
                code.append(f'(local.set $p64 (call $allocate64 (i64.const {size64}) (i64.const {align64})))')
                code.append(abi.store(parameters, 64, '(local.get $p64)', [f'(local.get $a{i})' for i in range(len(flat64))]))
            size32, align32 = abi.layout(parameters, 32)
            code.append(f'(local.set $p32 (call $allocate32 (i32.const {size32}) (i32.const {align32})))')
            code.append(f'(call {abi.copy(parameters, 64)} (local.get $p64) (local.get $p32))')
        host_args = ['(local.get $p32)'] if indirect else abi.load(parameters, 32, '(local.get $p32)')
        if retptr:
            size, alignment = abi.layout(result, 32)
            code.append(f'(local.set $r32 (call $allocate32 (i32.const {size}) (i32.const {alignment})))')
            host_args.append('(local.get $r32)')
        call = f'(call $host{index} ' + ' '.join(host_args) + ')'
        code.append(f'(local.set $value {call})' if returns64 else call)
        if retptr:
            code.append(f'(call {abi.copy(result, 32)} (local.get $r32) (local.get $a{len(args64) - 1}))')
        if flat64 and not indirect:
            code.append(f'(drop (call $realloc64 (local.get $p64) (i64.const {size64}) (i64.const {align64}) (i64.const 0)))')
        code.append('(global.set $next (local.get $saved))')
        if returns64:
            code.append('(local.get $value)')
        wrappers.append(f'(func (export {quote(exported)}) {arguments} {signature([], returns64)} {locals_}\n' + '\n'.join(code) + ')')

    flags, minimum, maximum = engine.memories[0]
    assert flags & 4 and not flags & 2, 'expected unshared memory64'
    memory = f'i64 {minimum}' + (f' {maximum}' if maximum is not None else '')
    heap = f'(import "env" "memory" (memory $heap {memory}))' if browser else f'(memory $heap (export "heap") {memory})'
    # Memory32 remains the canonical memory named "memory". Browser heaps are
    # imported so the application retains its usual control over growth.
    memory_declarations = (heap if browser else '') + '(memory $abi (export "memory") 1 32768)' + ('' if browser else heap)
    run_name = 'wasi:cli/run@0.2.0#run'
    run = '' if browser else f'(func (export {quote(run_name)}) (result i32) (call_indirect (type $run-type) (i32.const 1)))'
    source = '(module\n' + '\n'.join(imports) + memory_declarations + '''
      (type $alloc-type (func (param i64 i64 i64 i64) (result i64)))
      (type $run-type (func (result i32)))
      (table (export "engine-functions") 2 funcref)
      (global $next (mut i32) (i32.const 1024))
      (func $realloc64 (param i64 i64 i64 i64) (result i64)
        (call_indirect (type $alloc-type) (local.get 0) (local.get 1) (local.get 2) (local.get 3) (i32.const 0)))
      (func $allocate64 (param $size i64) (param $align i64) (result i64)
        (call $realloc64 (i64.const 0) (i64.const 0) (local.get $align) (local.get $size)))
      (func $allocate32 (param $size i32) (param $align i32) (result i32)
        (local $ptr i32) (local $end i64) (local $pages i32)
        (local.set $ptr (i32.and
          (i32.add (global.get $next) (i32.sub (local.get $align) (i32.const 1)))
          (i32.sub (i32.const 0) (local.get $align))))
        (local.set $end (i64.add (i64.extend_i32_u (local.get $ptr)) (i64.extend_i32_u (local.get $size))))
        (if (i64.gt_u (local.get $end) (i64.const 2147483647)) (then unreachable))
        (local.set $pages (i32.wrap_i64 (i64.shr_u (i64.add (local.get $end) (i64.const 65535)) (i64.const 16))))
        (if (i32.gt_u (local.get $pages) (memory.size $abi)) (then
          (if (i32.eq (memory.grow $abi (i32.sub (local.get $pages) (memory.size $abi))) (i32.const -1)) (then unreachable))))
        (global.set $next (i32.wrap_i64 (local.get $end)))
        (local.get $ptr))
      (func (export "cabi_realloc") (param $old i32) (param $oldsize i32) (param $align i32) (param $size i32) (result i32)
        (local $ptr i32) (local $length i32)
        (if (i32.eqz (local.get $size)) (then (return (i32.const 0))))
        (local.set $ptr (call $allocate32 (local.get $size) (local.get $align)))
        (local.set $length (select (local.get $oldsize) (local.get $size) (i32.lt_u (local.get $oldsize) (local.get $size))))
        (if (local.get $old) (then (memory.copy $abi $abi (local.get $ptr) (local.get $old) (local.get $length))))
        (local.get $ptr))
    ''' + '\n'.join(wrappers) + '\n'.join(body for _, body in abi.copies.values()) + run + ')'
    return source, names


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
        # ordinary Preview 2 imports of the bridge. No canonical operation
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
        definitions = ['(core module $engine64 ' + '\n'.join(stub_imports) + '''
          (func (export "cabi_realloc") (param i64 i64 i64 i64) (result i64) i64.const 0)
          (func (export "wasi:cli/run@0.2.0#run") (result i32) i32.const 0))''']
        definitions.append('(alias core export $main "heap" (core memory $engine-heap))')
        definitions.append('(alias core export $main "engine-functions" (core table $engine-functions))')
        groups = {}
        for index, (module, name, exported) in enumerate(names):
            definitions.append(f'(alias core export $main {quote(exported)} (core func $engine-import{index}))')
            groups.setdefault(module, []).append(f'(export {quote(name)} (func $engine-import{index}))')
        arguments = ['(with "env" (instance (export "memory" (memory $engine-heap))))']
        arguments += [f'(with {quote(module)} (instance ' + ' '.join(exports) + '))' for module, exports in groups.items()]
        definitions.append('(core instance $engine64 (instantiate $engine64 ' + ' '.join(arguments) + '))')
        definitions.append('''(core module $connect-engine
          (import "bridge" "table" (table 2 funcref))
          (import "engine" "cabi_realloc" (func $alloc (param i64 i64 i64 i64) (result i64)))
          (import "engine" "wasi:cli/run@0.2.0#run" (func $run (result i32)))
          (elem (i32.const 0) func $alloc $run))
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
