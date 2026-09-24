"""Build plugins for each precision and check rejection before entry points run."""
import argparse
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
    parser.add_argument("--host", required=True)
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
                subprocess.run([
                    args.host, str(plugin_path), kind,
                    str(sample_size), str(double_size),
                ], check=True, capture_output=True, text=True)
        print("Plugin precision checks passed")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise
