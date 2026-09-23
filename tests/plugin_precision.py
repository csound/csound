"""Build plugins for each precision and check rejection before entry points run."""
import argparse
import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tempfile


GENERIC_PLUGIN = r'''
#include "csdl.h"
static int creates, inits, destroys;
PUBLIC int32_t csoundModuleCreate(CSOUND *csound) { ++creates; return 0; }
PUBLIC int32_t csoundModuleInit(CSOUND *csound) { ++inits; return 0; }
PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) { ++destroys; return 0; }
PUBLIC int entryCounts(void) { return creates + 10 * inits + 100 * destroys; }
'''

CPP_PLUGIN = r'''
#include "modload.h"
static int inits;
void csnd::on_load(csnd::Csound *) { ++inits; }
extern "C" PUBLIC int entryCounts(void) { return 10 * inits; }
'''

LINKAGE_PLUGIN = r'''
#include "csdl.h"
static OENTRY localops[] = {
  { "precision_test_opcode", sizeof(OPDS), 0, "", "", NULL, NULL }
};
LINKAGE
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--include-dir", required=True)
    parser.add_argument("--generated-include-dir", required=True)
    parser.add_argument("--library", required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="csound-plugin-precision-") as work:
        root = Path(work)
        (root / "float-version.h").write_text("")
        (root / "version.h").write_bytes(
            (Path(args.generated_include_dir) / "version.h").read_bytes())
        # Keep installed plugins out of this test's engine instances.
        empty = root / "empty"
        empty.mkdir()
        os.environ["OPCODE7DIR"] = os.environ["OPCODE7DIR64"] = str(empty)
        host = ctypes.CDLL(args.library)
        signatures = {
            "csoundCreate": ([ctypes.c_void_p, ctypes.c_char_p], ctypes.c_void_p),
            "csoundDestroy": ([ctypes.c_void_p], None),
            "csoundSetOption": ([ctypes.c_void_p, ctypes.c_char_p], ctypes.c_int),
            "csoundCompileOrc": ([ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int], ctypes.c_int),
            "csoundCreateMessageBuffer": ([ctypes.c_void_p, ctypes.c_int], None),
            "csoundGetFirstMessage": ([ctypes.c_void_p], ctypes.c_char_p),
            "csoundPopFirstMessage": ([ctypes.c_void_p], None),
            "csoundGetMessageCnt": ([ctypes.c_void_p], ctypes.c_int),
        }
        for name, (argtypes, restype) in signatures.items():
            func = getattr(host, name)
            func.argtypes, func.restype = argtypes, restype
        host_sizes = (host.csoundGetSizeOfCsFloat(), host.csoundGetSizeOfCsDouble())

        for definitions, sample_size, double_size in (
            (["-DUSE_DOUBLE"], 8, 8), ([], 4, 8), (["-DUSE_FLOAT"], 4, 4),
        ):
            for kind in ("current", "cpp", "linkage", "legacy", "version", "zero", "missing"):
                modern = kind in ("current", "cpp", "linkage")
                if not modern and double_size == 4:
                    continue  # Legacy plugins always used 64-bit doubles.
                name = f"{kind}-{sample_size}-{double_size}"
                source = root / (name + (".cpp" if kind == "cpp" else ".c"))
                plugin_path = root / (name + (".dylib" if sys.platform == "darwin" else ".so"))
                code = CPP_PLUGIN if kind == "cpp" else LINKAGE_PLUGIN if kind == "linkage" else GENERIC_PLUGIN
                info = {
                    "current": "CSOUND_MODULE_INFO",
                    "legacy": "(CS_VERSION << 16) | (CS_SUBVER << 8) | sizeof(cs_float)",
                    "version": "(CS_VERSION << 16) | (CS_SUBVER << 8)",
                    "zero": "0",
                }.get(kind)
                if info is not None:
                    code += f"\nPUBLIC int32_t csoundModuleInfo(void) {{ return {info}; }}\n"
                source.write_text(code)
                subprocess.run([
                    args.cxx if kind == "cpp" else args.cc,
                    "-std=c++17" if kind == "cpp" else "-std=c11",
                    "-shared", "-fPIC", "-I" + str(root), "-I" + args.include_dir,
                    *definitions, str(source), "-o", str(plugin_path),
                ], check=True, capture_output=True, text=True)
                plugin = ctypes.CDLL(str(plugin_path))
                if modern:
                    # This also pins the legacy byte values for older loaders.
                    expected_byte = sample_size | (0x80 if double_size == 4 else 0)
                    assert plugin.csoundModuleInfo() & 0xff == expected_byte, name
                declared_double_size = double_size if modern else 8
                declares_sample = modern or kind == "legacy"
                accepted = host_sizes[1] == declared_double_size and (
                    not declares_sample or host_sizes[0] == sample_size)
                csound = host.csoundCreate(None, None)
                assert csound, name
                host.csoundCreateMessageBuffer(csound, 0)
                try:
                    option = ("--opcode-lib=" + str(plugin_path)).encode()
                    assert host.csoundSetOption(csound, option) == 0, name
                    orc = (b"instr 1\nprecision_test_opcode\nendin\n" if kind == "linkage"
                           else b"instr 1\nendin\n")
                    result = host.csoundCompileOrc(csound, orc, 0)
                    messages = []
                    while host.csoundGetMessageCnt(csound):
                        messages.append(host.csoundGetFirstMessage(csound).decode())
                        host.csoundPopFirstMessage(csound)
                    # A rejected library warns; only a missing opcode prevents compilation.
                    assert (result == 0) == (accepted or kind != "linkage"), (
                        name, host_sizes, result, messages)
                    if kind != "linkage":
                        expected = (10 if kind == "cpp" else 11) if accepted else 0
                        assert plugin.entryCounts() == expected, (name, plugin.entryCounts())
                    if not accepted:
                        assert "incompatible" in "".join(messages), (name, messages)
                finally:
                    host.csoundDestroy(csound)
                if kind not in ("cpp", "linkage"):
                    assert plugin.entryCounts() == (111 if accepted else 0), name
        print(f"Plugin precision checks passed for host {host_sizes}")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise
