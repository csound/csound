#!/usr/bin/env python3

import argparse
from pathlib import Path
import subprocess
import tempfile


WASM_PAGE_SIZE = 64 * 1024
PLUGIN_GLOBAL_BASE = 128 * 1024 * 1024
HOST_TABLE_ENTRIES = 3837
PLUGIN_TABLE_BASE = 4096


def read_uleb(module, position, end):
    value = 0
    shift = 0
    while position < end and shift < 70:
        byte = module[position]
        position += 1
        value |= (byte & 0x7F) << shift
        if byte & 0x80 == 0:
            return value, position
        shift += 7
    raise RuntimeError("invalid unsigned LEB128 value")


def write_uleb(value):
    if value < 0:
        raise ValueError("cannot encode a negative unsigned LEB128 value")
    encoded = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            byte |= 0x80
        encoded.append(byte)
        if not value:
            return bytes(encoded)


def read_name(module, position, end):
    size, position = read_uleb(module, position, end)
    name_end = position + size
    if name_end > end:
        raise RuntimeError("Wasm name extends past its section")
    return module[position:name_end], name_end


def read_limits(module, position, end):
    flags, position = read_uleb(module, position, end)
    if flags & ~0x01:
        raise RuntimeError("fixture has unsupported Wasm limits")
    minimum, position = read_uleb(module, position, end)
    if flags & 0x01:
        _, position = read_uleb(module, position, end)
    return minimum, position


def imported_minima(module):
    if module[:8] != b"\x00asm\x01\x00\x00\x00":
        raise RuntimeError("compiler did not produce a Wasm module")

    memory_minimum = None
    table_minimum = None
    import_sections = 0
    position = 8
    while position < len(module):
        section_id = module[position]
        position += 1
        section_size, position = read_uleb(module, position, len(module))
        section_end = position + section_size
        if section_end > len(module):
            raise RuntimeError("Wasm section extends past the module")
        if section_id == 2:
            import_sections += 1
            count, position = read_uleb(module, position, section_end)
            for _ in range(count):
                import_module, position = read_name(module, position, section_end)
                import_name, position = read_name(module, position, section_end)
                if position >= section_end:
                    raise RuntimeError("Wasm import has no kind")
                kind = module[position]
                position += 1
                if (
                    import_module == b"env"
                    and import_name == b"memory"
                    and kind == 2
                    and memory_minimum is None
                ):
                    memory_minimum, position = read_limits(
                        module, position, section_end
                    )
                elif (
                    import_module == b"env"
                    and import_name == b"__indirect_function_table"
                    and kind == 1
                    and table_minimum is None
                ):
                    if position >= section_end or module[position] != 0x70:
                        raise RuntimeError("fixture table is not a funcref table")
                    position += 1
                    table_minimum, position = read_limits(
                        module, position, section_end
                    )
                else:
                    name = import_module + b"." + import_name
                    raise RuntimeError(
                        f"fixture has unsupported Wasm import {name!r}"
                    )
            if position != section_end:
                raise RuntimeError("Wasm import section has trailing data")
        position = section_end

    if import_sections != 1 or memory_minimum is None or table_minimum is None:
        raise RuntimeError("fixture must import one memory and one function table")
    return memory_minimum, table_minimum


def custom_section(name, contents):
    payload = write_uleb(len(name)) + name + contents
    return b"\x00" + write_uleb(len(payload)) + payload


def add_loader_metadata(module):
    memory_minimum, table_minimum = imported_minima(module)
    memory_size = memory_minimum * WASM_PAGE_SIZE
    if memory_size < PLUGIN_GLOBAL_BASE:
        raise RuntimeError("fixture memory starts below the plugin global base")
    if table_minimum <= PLUGIN_TABLE_BASE:
        raise RuntimeError("fixture table does not extend past the plugin table base")

    memory_bytes = memory_size - PLUGIN_GLOBAL_BASE
    table_entries = table_minimum - HOST_TABLE_ENTRIES
    dylink = b"".join(
        (
            write_uleb(memory_bytes),
            write_uleb(0),
            write_uleb(table_entries),
            write_uleb(0),
            write_uleb(0),
        )
    )
    marker = custom_section(b"OPCODE.WASM", b"Built by OPCODE.WASM")
    layout = custom_section(b"dylink", dylink)
    return module[:8] + marker + layout + module[8:]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--source-include", required=True)
    parser.add_argument("--generated-include", required=True)
    parser.add_argument("--top-include", required=True)
    parser.add_argument("--sysroot")
    parser.add_argument("--use-double", action="store_true")
    args = parser.parse_args()

    output = Path(args.output).resolve()
    common = [
        args.compiler,
        "--target=wasm32-unknown-wasi",
        "-fPIC",
        "-fno-builtin",
        # csoundCore.h needs the jmp_buf type, but this fixture never calls
        # setjmp. Do not require Clang's private WebAssembly SJLJ switch.
        "-D__wasm_exception_handling__=1",
        "-I",
        str(Path(args.generated_include).resolve()),
        "-I",
        str(Path(args.source_include).resolve()),
        "-I",
        str(Path(args.top_include).resolve()),
    ]
    if args.sysroot:
        common.append(f"--sysroot={Path(args.sysroot).resolve()}")
    if args.use_double:
        common.append("-DUSE_DOUBLE=1")

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="csound-wasm-fixture-") as root:
        root = Path(root)
        object_file = root / "velvetlp.wasm.fixture.o"
        raw_module = root / "velvetlp.wasm"
        subprocess.run(
            [
                *common,
                "-std=c11",
                "-O2",
                "-c",
                str(Path(args.source).resolve()),
                "-o",
                str(object_file),
            ],
            check=True,
        )
        subprocess.run(
            [
                *common,
                "-nostdlib",
                "-Wl,--no-entry",
                "-Wl,--export-all",
                "-Wl,--import-memory",
                "-Wl,--import-table",
                f"-Wl,--global-base={PLUGIN_GLOBAL_BASE}",
                f"-Wl,--table-base={PLUGIN_TABLE_BASE}",
                "-Wl,-z,stack-size=131072",
                str(object_file),
                "-o",
                str(raw_module),
            ],
            check=True,
        )
        output.write_bytes(add_loader_metadata(raw_module.read_bytes()))


if __name__ == "__main__":
    main()
