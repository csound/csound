"""Canonical ABI layouts and copies for the pinned synchronous WASI 0.2 WIT.

Only pointers and list lengths change width. Handles, flags and scalar integer
fields retain their WIT widths. Unknown types fail the build.
"""
from functools import lru_cache


def align(value, alignment):
    return (value + alignment - 1) & -alignment


def const(bits, value):
    return f'(i{bits}.const {value})'


def add(bits, pointer, offset):
    return pointer if not offset else f'(i{bits}.add {pointer} {const(bits, offset)})'


def cast(expression, source, target):
    if source == target:
        return expression
    if source[0] == 'f':
        expression = f'(i{source[1:]}.reinterpret_{source} {expression})'
        source = 'i' + source[1:]
    width = 'i' + target[1:]
    if source != width:
        expression = f'({"i64.extend_i32_u" if width == "i64" else "i32.wrap_i64"} {expression})'
    if target[0] == 'f':
        expression = f'({target}.reinterpret_{width} {expression})'
    return expression


class ABI:
    def __init__(self, wit):
        self.wit = wit
        self.types = wit['types'][:]
        self.copies = {}

    def tuple(self, types):
        key = len(self.types)
        self.types.append({'kind': {'tuple': {'types': types}}})
        return key

    def kind(self, ty):
        if ty is None:
            return 'unit', None
        if isinstance(ty, str):
            return ty, None
        kind = self.types[ty]['kind']
        if isinstance(kind, str):
            return kind, None
        name, data = next(iter(kind.items()))
        if name == 'type':
            return self.kind(data)
        return name, data

    def fields(self, ty):
        kind, data = self.kind(ty)
        if kind == 'tuple':
            return data['types']
        if kind == 'record':
            return [field['type'] for field in data['fields']]
        raise ValueError(kind)

    def cases(self, ty):
        kind, data = self.kind(ty)
        if kind == 'option':
            return [None, data]
        if kind == 'result':
            return [data['ok'], data['err']]
        return [case['type'] for case in data['cases']]

    @lru_cache(None)
    def layout(self, ty, bits):
        kind, data = self.kind(ty)
        if kind == 'unit':
            return 0, 1
        if kind in ('string', 'list'):
            return bits // 4, bits // 8
        if kind in ('record', 'tuple'):
            size, alignment = 0, 1
            for field in self.fields(ty):
                length, a = self.layout(field, bits)
                size = align(size, a) + length
                alignment = max(alignment, a)
            return align(size, alignment), alignment
        if kind in ('option', 'result', 'variant'):
            cases = self.cases(ty)
            assert len(cases) <= 256
            layouts = [self.layout(case, bits) for case in cases]
            a = max(a for _, a in layouts)
            return align(a + max(size for size, _ in layouts), a), a
        if kind == 'flags':
            count = len(data['flags'])
            if count <= 8:
                return 1, 1
            if count <= 16:
                return 2, 2
            return 4 * ((count + 31) // 32), 4
        if kind == 'enum':
            assert len(data['cases']) <= 256
            return 1, 1
        sizes = {'bool': 1, 'u8': 1, 's8': 1, 'u16': 2, 's16': 2,
                 'u32': 4, 's32': 4, 'char': 4, 'f32': 4,
                 'u64': 8, 's64': 8, 'f64': 8, 'resource': 4, 'handle': 4}
        size = sizes[kind]
        return size, size

    def offsets(self, ty, bits):
        offset = 0
        result = []
        for field in self.fields(ty):
            size, a = self.layout(field, bits)
            offset = align(offset, a)
            result.append(offset)
            offset += size
        return result

    @lru_cache(None)
    def flat(self, ty, bits):
        kind, data = self.kind(ty)
        if kind == 'unit':
            return ()
        if kind in ('string', 'list'):
            return (f'i{bits}', f'i{bits}')
        if kind in ('tuple', 'record'):
            return sum((self.flat(field, bits) for field in self.fields(ty)), ())
        if kind in ('option', 'result', 'variant'):
            cases = [self.flat(case, bits) for case in self.cases(ty)]
            slots = []
            for index in range(max(map(len, cases))):
                types = {case[index] for case in cases if index < len(case)}
                slots.append(next(iter(types)) if len(types) == 1 else
                             'i64' if types & {'i64', 'f64'} else 'i32')
            return ('i32', *slots)
        if kind == 'flags':
            return ('i32',) * max(1, (len(data['flags']) + 31) // 32)
        return ({'u64': 'i64', 's64': 'i64', 'f32': 'f32', 'f64': 'f64'}.get(kind, 'i32'),)

    def store(self, ty, bits, ptr, values):
        kind, _ = self.kind(ty)
        mem = '$heap' if bits == 64 else '$abi'
        if kind == 'unit':
            return ''
        if kind in ('record', 'tuple'):
            code, index = [], 0
            for field, offset in zip(self.fields(ty), self.offsets(ty, bits)):
                count = len(self.flat(field, bits))
                code.append(self.store(field, bits, add(bits, ptr, offset), values[index:index + count]))
                index += count
            return '\n'.join(code)
        if kind in ('option', 'result', 'variant'):
            slots = self.flat(ty, bits)
            code = [f'(i32.store8 {mem} {ptr} {values[0]})']
            for index, case in enumerate(self.cases(ty)):
                values_case = [cast(v, src, dst) for v, src, dst in zip(values[1:], slots[1:], self.flat(case, bits))]
                payload = self.store(case, bits, add(bits, ptr, self.layout(ty, bits)[1]), values_case)
                code.append(f'(if (i32.eq {values[0]} (i32.const {index})) (then {payload}))')
            return '\n'.join(code)
        if kind in ('string', 'list'):
            return '\n'.join(f'(i{bits}.store {mem} {add(bits, ptr, i * bits // 8)} {value})' for i, value in enumerate(values))
        size = self.layout(ty, bits)[0]
        flat = self.flat(ty, bits)
        if kind == 'flags' and size > 4:
            return '\n'.join(f'(i32.store {mem} {add(bits, ptr, i * 4)} {v})' for i, v in enumerate(values))
        operation = flat[0] + (f'.store{size * 8}' if size < 4 else '.store')
        return f'({operation} {mem} {ptr} {values[0]})'

    def load(self, ty, bits, ptr):
        kind, _ = self.kind(ty)
        mem = '$heap' if bits == 64 else '$abi'
        if kind == 'unit':
            return []
        if kind in ('record', 'tuple'):
            return sum((self.load(field, bits, add(bits, ptr, offset)) for field, offset in zip(self.fields(ty), self.offsets(ty, bits))), [])
        if kind in ('option', 'result', 'variant'):
            tag = f'(i32.load8_u {mem} {ptr})'
            cases = self.cases(ty)
            flats = [self.flat(case, bits) for case in cases]
            loaded = [self.load(case, bits, add(bits, ptr, self.layout(ty, bits)[1])) for case in cases]
            result = [tag]
            for slot, target in enumerate(self.flat(ty, bits)[1:]):
                expression = '(unreachable)'
                for index in reversed(range(len(cases))):
                    value = cast(loaded[index][slot], flats[index][slot], target) if slot < len(loaded[index]) else f'({target}.const 0)'
                    expression = f'(if (result {target}) (i32.eq {tag} (i32.const {index})) (then {value}) (else {expression}))'
                result.append(expression)
            return result
        if kind in ('string', 'list'):
            return [f'(i{bits}.load {mem} {add(bits, ptr, i * bits // 8)})' for i in range(2)]
        size = self.layout(ty, bits)[0]
        flat = self.flat(ty, bits)
        if kind == 'flags' and size > 4:
            return [f'(i32.load {mem} {add(bits, ptr, i * 4)})' for i in range(size // 4)]
        operation = flat[0] + (f'.load{size * 8}_u' if size < 4 else '.load')
        return [f'({operation} {mem} {ptr})']

    def copy(self, ty, source_bits):
        key = (ty, source_bits)
        if key in self.copies:
            return self.copies[key][0]
        name = f'$copy{len(self.copies)}'
        self.copies[key] = name, ''
        target_bits = 96 - source_bits
        source_mem, target_mem = ('$heap', '$abi') if source_bits == 64 else ('$abi', '$heap')
        src, dst = '(local.get $src)', '(local.get $dst)'
        kind, data = self.kind(ty)
        locals_, code = '', []
        if kind in ('tuple', 'record'):
            for field, off_s, off_d in zip(self.fields(ty), self.offsets(ty, source_bits), self.offsets(ty, target_bits)):
                fn = self.copy(field, source_bits)
                code.append(f'(call {fn} {add(source_bits, src, off_s)} {add(target_bits, dst, off_d)})')
        elif kind in ('option', 'result', 'variant'):
            tag = f'(i32.load8_u {source_mem} {src})'
            cases = self.cases(ty)
            code.append(f'(if (i32.ge_u {tag} (i32.const {len(cases)})) (then unreachable))')
            code.append(f'(i32.store8 {target_mem} {dst} {tag})')
            for index, case in enumerate(cases):
                fn = self.copy(case, source_bits)
                code.append(f'(if (i32.eq {tag} (i32.const {index})) (then (call {fn} {add(source_bits, src, self.layout(ty, source_bits)[1])} {add(target_bits, dst, self.layout(ty, target_bits)[1])})))')
        elif kind in ('string', 'list'):
            element = 'u8' if kind == 'string' else data
            size_s, _ = self.layout(element, source_bits)
            size_d, align_d = self.layout(element, target_bits)
            locals_ = f'(local $n i32) (local $i i32) (local $p i{source_bits}) (local $q i{target_bits})'
            length = f'(i{source_bits}.load {source_mem} {add(source_bits, src, source_bits // 8)})'
            limit = 0x7fffffff // max(size_s, size_d, 1)
            code += [f'(if (i{source_bits}.gt_u {length} {const(source_bits, limit)}) (then unreachable))',
                     f'(local.set $n {cast(length, f"i{source_bits}", "i32")})',
                     f'(local.set $p (i{source_bits}.load {source_mem} {src}))']
            byte_length = f'(i{target_bits}.mul {cast("(local.get $n)", "i32", f"i{target_bits}")} {const(target_bits, size_d)})'
            allocation = f'(call $allocate32 {byte_length} (i32.const {align_d}))' if target_bits == 32 else f'(call $allocate64 {byte_length} (i64.const {align_d}))'
            code += [f'(local.set $q {allocation})',
                     f'(i{target_bits}.store {target_mem} {dst} (local.get $q))',
                     f'(i{target_bits}.store {target_mem} {add(target_bits, dst, target_bits // 8)} {cast("(local.get $n)", "i32", f"i{target_bits}")})']
            if element == 'u8':
                code.append(f'(memory.copy {target_mem} {source_mem} (local.get $q) (local.get $p) (local.get $n))')
            else:
                fn = self.copy(element, source_bits)
                code.append(f'''(block $done (loop $copy
                  (br_if $done (i32.eq (local.get $i) (local.get $n)))
                  (call {fn}
                    (i{source_bits}.add (local.get $p) (i{source_bits}.mul {cast('(local.get $i)', 'i32', f'i{source_bits}')} {const(source_bits, size_s)}))
                    (i{target_bits}.add (local.get $q) (i{target_bits}.mul {cast('(local.get $i)', 'i32', f'i{target_bits}')} {const(target_bits, size_d)})))
                  (local.set $i (i32.add (local.get $i) (i32.const 1))) (br $copy)))''')
        else:
            size = self.layout(ty, source_bits)[0]
            assert size == self.layout(ty, target_bits)[0]
            if size:
                code.append(f'(memory.copy {target_mem} {source_mem} {dst} {src} (i32.const {size}))')
        function = f'(func {name} (param $src i{source_bits}) (param $dst i{target_bits}) {locals_}\n' + '\n'.join(code) + ')'
        self.copies[key] = name, function
        return name
