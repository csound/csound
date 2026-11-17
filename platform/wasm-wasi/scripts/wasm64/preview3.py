"""Lower Preview 3 imports through memory32 without shortening engine pointers.

Each operation owns an arena until it returns or its waitable completes. Host
allocations inside returned lists have separate ownership and are freed after
copying, including when a wait finishes a previously blocked operation.
"""
import re
from abi import ABI
from wasm import quote


def signature(params, results):
    return ('(param ' + ' '.join(params) + ')' if params else '') + ('(result ' + ' '.join(results) + ')' if results else '')


def generate(engine, wit, browser):
    abi = ABI(wit, free_source=True)
    interfaces = {}
    for interface in wit['interfaces']:
        package, _, version = wit['packages'][interface['package']]['name'].partition('@')
        interfaces[f'{package}/{interface["name"]}@{version}'] = interface
    imports, wrappers, names, callbacks = [], [], [], []

    def imported(module, name, params, returns):
        index = len(names)
        exported = f'{module}#{name}'
        names.append((module, name, exported))
        imports.append(f'(import {quote(module)} {quote(name)} (func $host{index} {signature(params, returns)}))')
        return index, exported

    def wrap(exported, params, returns, body, locals_=''):
        args = ' '.join(f'(param $a{i} {ty})' for i, ty in enumerate(params))
        wrappers.append(f'(func (export {quote(exported)}) {args} {signature([], returns)} {locals_}\n{body})')

    def frame_start():
        return ['(local.set $previous (global.get $arena))',
                '(local.set $frame (call $malloc32 (i32.const 48)))',
                '(memory.fill $abi (local.get $frame) (i32.const 0) (i32.const 48))',
                '(global.set $arena (local.get $frame))']

    def frame_end():
        return ['(call $freeFrame (local.get $frame))', '(global.set $arena (local.get $previous))']

    frame_locals = '(local $frame i32) (local $previous i32)'

    def callback(result, mode):
        index = len(callbacks)
        source = '(i32.load $abi offset=16 (local.get $frame))'
        target = '(i64.load $abi offset=24 (local.get $frame))'
        state = '(i32.and (local.get $status) (i32.const 15))'
        if mode == 'subtask':
            code = f'(if (i32.eq {state} (i32.const 2)) (then (call {abi.copy(result, 32)} {source} {target})))' if result is not None else ''
        elif mode == 'future-read':
            code = f'(if (i32.eqz {state}) (then (call {abi.copy(result, 32)} {source} {target})))'
        elif mode == 'stream-read':
            size32 = abi.layout(result, 32)[0]
            size64 = abi.layout(result, 64)[0]
            code = f'''(local.set $count (i32.shr_u (local.get $status) (i32.const 4)))
              (if (i32.gt_u (local.get $count) (i32.load $abi offset=32 (local.get $frame))) (then unreachable))
              (block $done (loop $copy
                (br_if $done (i32.eq (local.get $index) (local.get $count)))
                (call {abi.copy(result, 32)}
                  (i32.add {source} (i32.mul (local.get $index) (i32.const {size32})))
                  (i64.add {target} (i64.mul (i64.extend_i32_u (local.get $index)) (i64.const {size64}))))
                (local.set $index (i32.add (local.get $index) (i32.const 1))) (br $copy)))'''
        else:
            code = ''
        callbacks.append(f'(func $callback{index} (param $frame i32) (param $status i32) (local $index i32) (local $count i32) {code})')
        return index

    def retain(code, cb, handle):
        code += [f'(i32.store $abi offset=12 (local.get $frame) (i32.const {cb}))',
                 f'(call $retain (local.get $frame) {handle})',
                 '(global.set $arena (local.get $previous))']

    for module, name, kind, type_index in engine.imports:
        if kind != 0:
            continue
        args64, returns64 = engine.types[type_index]
        if module == 'env' and name in ('__wasm_get_stack_pointer', '__wasm_set_stack_pointer', '__wasm_get_tls_base', '__wasm_set_tls_base'):
            global_ = '$stack' if 'stack_pointer' in name else '$tls'
            exported = module + '#' + name
            names.append((module, name, exported))
            body = f'(global.get {global_})' if '_get_' in name else f'(global.set {global_} (local.get $a0))'
            wrap(exported, args64, returns64, body)
            continue
        if not (module.startswith('wasi:') or module == '$root'):
            if browser:
                continue
            raise ValueError(f'Unsupported command import: {module}#{name}')
        # Task context pointers belong to the memory64 task, not canonical memory.
        if module == '$root' and name in ('[context-get-0]', '[context-set-0]'):
            exported = module + '#' + name
            names.append((module, name, exported))
            wrap(exported, args64, returns64, '(global.get $context)' if 'get' in name else '(global.set $context (local.get $a0))')
            continue
        intrinsic = re.fullmatch(r'(?:\[async-lower\])?\[(stream|future)-(new|read|write|cancel-read|cancel-write|drop-readable|drop-writable)-(\d+)\](.*)', name)
        wait = module == '$root' and name in ('[waitable-set-wait]', '[waitable-set-poll]')
        passthrough = (module == '$root' and not wait) or name.startswith('[resource-drop]') or (intrinsic and intrinsic[2] not in ('read', 'write'))
        if passthrough:
            assert 'i64' not in args64, (module, name, args64)
            index, exported = imported(module, name, args64, returns64)
            call = f'(call $host{index} ' + ' '.join(f'(local.get $a{i})' for i in range(len(args64))) + ')'
            cancel = 'cancel-' in name or name == '[subtask-cancel]'
            body = f'(local.set $value {call}) (call $completePending (local.get $a0) (local.get $value)) (local.get $value)' if cancel else call
            wrap(exported, args64, returns64, body, '(local $value i32)' if cancel else '')
            continue
        if wait:
            assert (args64, returns64) == (['i32', 'i64'], ['i32'])
            index, exported = imported(module, name, ['i32', 'i32'], ['i32'])
            code = frame_start() + ['(local.set $result (call $allocate32 (i32.const 8) (i32.const 4)))',
                f'(local.set $value (call $host{index} (local.get $a0) (local.get $result)))',
                '(if (local.get $value) (then',
                '(call $completePending (i32.load $abi (local.get $result)) (i32.load $abi offset=4 (local.get $result)))',
                '(memory.copy $heap $abi (local.get $a1) (local.get $result) (i32.const 8))))'] + frame_end() + ['(local.get $value)']
            wrap(exported, args64, returns64, '\n'.join(code), frame_locals + '(local $result i32) (local $value i32)')
            continue
        if intrinsic:
            stream_kind, operation, ordinal, function_name = intrinsic.groups()
            function = interfaces[module]['functions'][function_name]
            streams = []
            def visit(ty):
                k, data = abi.kind(ty)
                if k in ('stream', 'future'):
                    streams.append((k, data))
                    visit(data)
                elif k in ('tuple', 'record'):
                    for child in abi.fields(ty): visit(child)
                elif k in ('variant', 'result', 'option'):
                    for child in abi.cases(ty): visit(child)
            for parameter in function['params']: visit(parameter['type'])
            visit(function.get('result'))
            actual_kind, payload = streams[int(ordinal)]
            assert actual_kind == stream_kind
            expected = ['i32', 'i64'] + (['i64'] if stream_kind == 'stream' else [])
            assert (args64, returns64) == (expected, ['i32'])
            args32 = ['i32', 'i32'] + (['i32'] if stream_kind == 'stream' else [])
            index, exported = imported(module, name, args32, ['i32'])
            size32, alignment = abi.layout(payload, 32)
            size64 = abi.layout(payload, 64)[0]
            code = frame_start()
            if stream_kind == 'stream':
                code += [f'(if (i64.gt_u (local.get $a2) (i64.const {min(0x0fffffff, 0x7fffffff // max(size32, size64, 1))})) (then unreachable))',
                         '(local.set $count (i32.wrap_i64 (local.get $a2)))']
            else: code += ['(local.set $count (i32.const 1))']
            code += [f'(local.set $result (call $allocate32 (i32.mul (local.get $count) (i32.const {size32})) (i32.const {alignment})))',
                     '(i32.store $abi offset=16 (local.get $frame) (local.get $result))',
                     '(i64.store $abi offset=24 (local.get $frame) (local.get $a1))',
                     '(i32.store $abi offset=32 (local.get $frame) (local.get $count))']
            if operation == 'write':
                code += [f'''(block $done (loop $copy
                  (br_if $done (i32.eq (local.get $index) (local.get $count)))
                  (call {abi.copy(payload, 64)}
                    (i64.add (local.get $a1) (i64.mul (i64.extend_i32_u (local.get $index)) (i64.const {size64})))
                    (i32.add (local.get $result) (i32.mul (local.get $index) (i32.const {size32}))))
                  (local.set $index (i32.add (local.get $index) (i32.const 1))) (br $copy)))''']
            code += [f'(local.set $value (call $host{index} (local.get $a0) (local.get $result)' + (' (local.get $count)' if stream_kind == 'stream' else '') + '))']
            cb = callback(payload, stream_kind + '-' + operation)
            code += ['(if (i32.eq (local.get $value) (i32.const -1)) (then']
            retain(code, cb, '(local.get $a0)')
            code += [') (else', f'(call $callback{cb} (local.get $frame) (local.get $value))'] + frame_end() + ['))', '(local.get $value)']
            wrap(exported, args64, returns64, '\n'.join(code), frame_locals + '(local $result i32) (local $value i32) (local $count i32) (local $index i32)')
            continue
        async_ = name.startswith('[async-lower]')
        function = interfaces[module]['functions'][name.removeprefix('[async-lower]')]
        parameters = abi.tuple([p['type'] for p in function['params']])
        result = function.get('result')
        flat64, flat32 = abi.flat(parameters, 64), abi.flat(parameters, 32)
        result64, result32 = abi.flat(result, 64), abi.flat(result, 32)
        indirect = len(flat64) > 16
        retptr = result is not None if async_ else len(result64) > 1
        expected = (['i64'] if indirect else list(flat64)) + (['i64'] if retptr else [])
        result_types = ['i32'] if async_ else [] if retptr else list(result64)
        assert (args64, returns64) == (expected, result_types), (module, name, (args64, returns64), (expected, result_types))
        args32 = (['i32'] if indirect else list(flat32)) + (['i32'] if retptr else [])
        returns32 = ['i32'] if async_ else [] if retptr else list(result32)
        index, exported = imported(module, name, args32, returns32)
        code = frame_start()
        if flat64:
            size64, align64 = abi.layout(parameters, 64)
            size32, align32 = abi.layout(parameters, 32)
            if indirect: code += ['(local.set $p64 (local.get $a0))']
            else:
                code += [f'(local.set $p64 (call $allocate64 (i64.const {size64}) (i64.const {align64})))',
                         abi.store(parameters, 64, '(local.get $p64)', [f'(local.get $a{i})' for i in range(len(flat64))])]
            code += [f'(local.set $p32 (call $allocate32 (i32.const {size32}) (i32.const {align32})))',
                     f'(call {abi.copy(parameters, 64)} (local.get $p64) (local.get $p32))']
            if not indirect: code += ['(call $free64 (local.get $p64))']
        host_args = ['(local.get $p32)'] if indirect else abi.load(parameters, 32, '(local.get $p32)')
        if retptr:
            size, alignment = abi.layout(result, 32)
            code += [f'(local.set $result (call $allocate32 (i32.const {size}) (i32.const {alignment})))',
                     '(i32.store $abi offset=16 (local.get $frame) (local.get $result))',
                     f'(i64.store $abi offset=24 (local.get $frame) (local.get $a{len(args64)-1}))']
            host_args += ['(local.get $result)']
        call = f'(call $host{index} ' + ' '.join(host_args) + ')'
        code += [f'(local.set $value {call})' if returns64 else call]
        if async_:
            cb = callback(result, 'subtask')
            code += ['(if (i32.eq (i32.and (local.get $value) (i32.const 15)) (i32.const 2)) (then',
                     f'(call $callback{cb} (local.get $frame) (local.get $value))'] + frame_end() + [') (else']
            retain(code, cb, '(i32.shr_u (local.get $value) (i32.const 4))')
            code += ['(i32.store $abi offset=36 (local.get $frame) (i32.const 1))', '))']
        else:
            if retptr: code += [f'(call {abi.copy(result, 32)} (local.get $result) (local.get $a{len(args64)-1}))']
            code += frame_end()
        if returns64: code += ['(local.get $value)']
        locals_ = frame_locals + '(local $p64 i64) (local $p32 i32) (local $result i32)'
        if returns64: locals_ += f'(local $value {returns64[0]})'
        wrap(exported, args64, returns64, '\n'.join(code), locals_)

    flags, minimum, maximum = engine.memories[0]
    assert flags & 4 and not flags & 2
    memory = f'i64 {minimum}' + (f' {maximum}' if maximum is not None else '')
    heap = f'(import "env" "memory" (memory $heap {memory}))' if browser else f'(memory $heap (export "heap") {memory})'
    memory_declarations = (heap if browser else '') + '(memory $abi (export "memory") 1 32768)' + ('' if browser else heap)
    run = ''
    if not browser:
        from bridge import command_export
        run = f'''(func (export {quote(command_export(engine))}) (result i32) (local $result i32)
          (call $taskHook (i32.const 0))
          (local.set $result (call_indirect $engine (type $run-type) (i32.const 1)))
          (call $taskHook (i32.const 1)) (local.get $result))'''
    dispatch = '\n'.join(f'(if (i32.eq (local.get $callback) (i32.const {i})) (then (call $callback{i} (local.get $frame) (local.get $status)) (return)))' for i in range(len(callbacks)))
    source = '(module\n' + '\n'.join(imports) + memory_declarations + RUNTIME + '\n'.join(wrappers) + '\n'.join(callbacks) + '\n'.join(body for _, body in abi.copies.values()) + f'''
      (func $dispatch (param $frame i32) (param $status i32)
        (local $callback i32)
        (local.set $callback (i32.load $abi offset=12 (local.get $frame)))
        {dispatch} unreachable)
      {run})'''
    return source, names


RUNTIME = r'''
(type $alloc-type (func (param i64 i64 i64 i64) (result i64)))
(type $free-type (func (param i64)))
(type $hook-type (func (param i32)))
(type $run-type (func (result i32)))
(table $engine (export "engine-functions") 4 funcref)
(global $stack (mut i64) (i64.const 0))
(global $tls (mut i64) (i64.const 0))
(global $context (mut i64) (i64.const 0))
(global $next (mut i32) (i32.const 1024))
(global $free (mut i32) (i32.const 0))
(global $arena (mut i32) (i32.const 0))
(global $pending (mut i32) (i32.const 0))
(func $allocate64 (param $size i64) (param $align i64) (result i64)
  (call_indirect $engine (type $alloc-type) (i64.const 0) (i64.const 0) (local.get $align) (local.get $size) (i32.const 0)))
(func $free64 (param i64)
  (call_indirect $engine (type $free-type) (local.get 0) (i32.const 2)))
(func $taskHook (param i32)
  (call_indirect $engine (type $hook-type) (local.get 0) (i32.const 3)))
(func $malloc32 (param $size i32) (result i32)
  (local $block i32) (local $previous i32) (local $end i64) (local $pages i32)
  (if (i32.eqz (local.get $size)) (then (return (i32.const 0))))
  (if (i32.gt_u (local.get $size) (i32.const 2147483584)) (then unreachable))
  (local.set $size (i32.and (i32.add (local.get $size) (i32.const 15)) (i32.const -16)))
  (local.set $block (global.get $free))
  (block $found (loop $search
    (br_if $found (i32.eqz (local.get $block)))
    (if (i32.ge_u (i32.load $abi (local.get $block)) (local.get $size)) (then
      (if (local.get $previous)
        (then (i32.store $abi offset=4 (local.get $previous) (i32.load $abi offset=4 (local.get $block))))
        (else (global.set $free (i32.load $abi offset=4 (local.get $block)))))
      (br $found)))
    (local.set $previous (local.get $block))
    (local.set $block (i32.load $abi offset=4 (local.get $block))) (br $search)))
  (if (i32.eqz (local.get $block)) (then
    (local.set $block (global.get $next))
    (local.set $end (i64.add (i64.extend_i32_u (local.get $block)) (i64.add (i64.extend_i32_u (local.get $size)) (i64.const 16))))
    (if (i64.gt_u (local.get $end) (i64.const 2147483647)) (then unreachable))
    (local.set $pages (i32.wrap_i64 (i64.shr_u (i64.add (local.get $end) (i64.const 65535)) (i64.const 16))))
    (if (i32.gt_u (local.get $pages) (memory.size $abi)) (then
      (if (i32.eq (memory.grow $abi (i32.sub (local.get $pages) (memory.size $abi))) (i32.const -1)) (then unreachable))))
    (global.set $next (i32.wrap_i64 (local.get $end)))
    (i32.store $abi (local.get $block) (local.get $size))))
  (i32.store $abi offset=12 (local.get $block) (i32.const 1))
  (i32.add (local.get $block) (i32.const 16)))
(func $free32 (param $pointer i32) (local $block i32)
  (if (i32.eqz (local.get $pointer)) (then (return)))
  (local.set $block (i32.sub (local.get $pointer) (i32.const 16)))
  (if (i32.ne (i32.load $abi offset=12 (local.get $block)) (i32.const 1)) (then unreachable))
  (i32.store $abi offset=12 (local.get $block) (i32.const 0))
  (i32.store $abi offset=4 (local.get $block) (global.get $free))
  (global.set $free (local.get $block)))
(func $allocate32 (param $size i32) (param $align i32) (result i32) (local $pointer i32)
  (if (i32.gt_u (local.get $align) (i32.const 16)) (then unreachable))
  (local.set $pointer (call $malloc32 (local.get $size)))
  (if (local.get $pointer) (then
    (i32.store $abi (i32.sub (local.get $pointer) (i32.const 8)) (i32.load $abi (global.get $arena)))
    (i32.store $abi (global.get $arena) (local.get $pointer))))
  (local.get $pointer))
(func $freeFrame (param $frame i32) (local $pointer i32) (local $next i32)
  (local.set $pointer (i32.load $abi (local.get $frame)))
  (block $done (loop $free
    (br_if $done (i32.eqz (local.get $pointer)))
    (local.set $next (i32.load $abi (i32.sub (local.get $pointer) (i32.const 8))))
    (call $free32 (local.get $pointer))
    (local.set $pointer (local.get $next)) (br $free)))
  (call $free32 (local.get $frame)))
(func $retain (param $frame i32) (param $handle i32)
  (i32.store $abi offset=4 (local.get $frame) (global.get $pending))
  (i32.store $abi offset=8 (local.get $frame) (local.get $handle))
  (global.set $pending (local.get $frame)))
(func $completePending (param $handle i32) (param $status i32)
  (local $frame i32) (local $previous i32) (local $next i32)
  (local.set $frame (global.get $pending))
  (block $done (loop $search
    (br_if $done (i32.eqz (local.get $frame)))
    (if (i32.eq (i32.load $abi offset=8 (local.get $frame)) (local.get $handle)) (then
      (if (i32.and (i32.load $abi offset=36 (local.get $frame))
          (i32.lt_u (i32.and (local.get $status) (i32.const 15)) (i32.const 2))) (then (return)))
      (call $dispatch (local.get $frame) (local.get $status))
      (local.set $next (i32.load $abi offset=4 (local.get $frame)))
      (if (local.get $previous)
        (then (i32.store $abi offset=4 (local.get $previous) (local.get $next)))
        (else (global.set $pending (local.get $next))))
      (call $freeFrame (local.get $frame)) (return)))
    (local.set $previous (local.get $frame))
    (local.set $frame (i32.load $abi offset=4 (local.get $frame))) (br $search))))
(func (export "cabi_realloc") (param $old i32) (param $oldsize i32) (param $align i32) (param $size i32) (result i32)
  (local $pointer i32) (local $length i32)
  (if (i32.gt_u (local.get $align) (i32.const 16)) (then unreachable))
  (local.set $pointer (call $malloc32 (local.get $size)))
  (local.set $length (select (local.get $oldsize) (local.get $size) (i32.lt_u (local.get $oldsize) (local.get $size))))
  (if (local.get $old) (then
    (memory.copy $abi $abi (local.get $pointer) (local.get $old) (local.get $length))
    (call $free32 (local.get $old))))
  (local.get $pointer))
'''
