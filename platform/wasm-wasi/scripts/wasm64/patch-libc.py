"""Apply the pinned wasi-libc fixes needed by LLVM's wasm64 target."""
from pathlib import Path

atomic = Path('libc-top-half/musl/arch/wasm32/atomic_arch.h')
atomic.write_text(atomic.read_text() + '\n#ifdef __wasm64__\n#define a_cas_p(p, t, s) (__sync_val_compare_and_swap((p), (t), (s)))\n#endif\n')
tls = Path('libc-top-half/musl/src/env/__init_tls.c')
source = tls.read_text()
assert source.count('".globaltype __stack_pointer, i32\\n"') == 1
source = source.replace('i32', 'i64')
tls.write_text(source)

# This object contains only WIT metadata and a no-op force-link function.
# Mark its target architecture so wasm-ld can link it into a memory64 module.
def leb(value):
    result = bytearray()
    while value >= 128:
        result.append((value & 127) | 128)
        value >>= 7
    result.append(value)
    return result

# These declarations describe pointers, not the task hook's i32 event code.
for filename in ('libc-bottom-half/crt/wasip3_symbol_references.h',
                 'libc-top-half/headers/private/wasi/wasip3_tls.h'):
    path = Path(filename)
    path.write_text(path.read_text().replace('i32', 'i64'))
path = Path('libc-bottom-half/sources/__wasm_task_hook.S')
source = path.read_text().replace('i32', 'i64')
source = source.replace('__wasilibc_task_hook (i64, i64, i64)', '__wasilibc_task_hook (i32, i64, i64)')
source = source.replace('__wasm_task_hook (i64)', '__wasm_task_hook (i32)')
path.write_text(source)

path = Path('libc-bottom-half/sources/wasip3_component_type.o')
name = b'target_features'
payload = leb(len(name)) + name + b'\x01+\x08memory64'
data = path.read_bytes()
# LLVM 21 also infers object bitness from a memory declaration. A memory-only
# import does not alter the force-link function's index or linking metadata.
entry = b'\x01\x03env\x0f__linear_memory\x02\x04\x00'
position = 8
while position < len(data):
    start = position
    kind = data[position]
    position += 1
    length = shift = 0
    while True:
        byte = data[position]
        position += 1
        length |= (byte & 127) << shift
        if byte < 128:
            break
        shift += 7
    if kind > 2:
        data = data[:start] + b'\x02' + leb(len(entry)) + entry + data[start:]
        break
    position += length
path.write_bytes(data + b'\0' + leb(len(payload)) + payload)
