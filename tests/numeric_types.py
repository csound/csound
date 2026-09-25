"""Compile public C/C++ headers in each precision mode and check CS7 aliases."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--include-dir", required=True)
    parser.add_argument("--generated-include-dir", required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="csound-numeric-types-") as work:
        root = Path(work)
        # An empty generated config lets each case choose its own precision.
        (root / "float-version.h").write_text("")
        (root / "version.h").write_bytes(
            (Path(args.generated_include_dir) / "version.h").read_bytes())
        # GNU C++11 exposes picolibc's C math names, as in the opcode build.
        for compiler, suffix, standard, assertion in (
            (args.cc, "c", "c11", "_Static_assert"),
            (args.cxx, "cpp", "gnu++11", "static_assert"),
        ):
            for definitions, float_size, double_size in (
                (["-DUSE_DOUBLE"], 8, 8),
                ([], 4, 8),
                (["-DUSE_FLOAT"], 4, 4),
            ):
                source = root / ("consumer." + suffix)
                # Match C++ opcodes that include cmath before Csound headers.
                math_include = "#include <cmath>" if suffix == "cpp" else ""
                math_scope = "using namespace std;" if suffix == "cpp" else ""
                modern = f'''
{math_include}
#include "csound.h"
#include "csdl.h"
{math_scope}
#include "arrays.h"
{assertion}(sizeof(cs_float) == {float_size}, "cs_float size");
{assertion}(sizeof(cs_double) == {double_size}, "cs_double size");
{assertion}(CS_VAR_TYPE_OFFSET == offsetof(CS_VAR_MEM, value), "value offset");
cs_float sample;
cs_double value;
'''
                command = [compiler, "-std=" + standard, "-fsyntax-only",
                           "-I" + str(root), "-I" + args.include_dir,
                           *definitions, str(source)]
                source.write_text(modern)
                subprocess.run([*command, "-Werror=deprecated-declarations"],
                               check=True, capture_output=True, text=True)
                source.write_text(modern + f'''
MYFLT legacy_sample;
{assertion}(sizeof(MYFLT) == sizeof(cs_float), "legacy alias size");
void legacy_host(CSOUND *csound) {{
    csoundSetControlChannel(csound, "gain", legacy_sample);
    (void)csoundGetSizeOfMYFLT();
    (void)csound->GetSizeOfMYFLT();
}}
''')
                result = subprocess.run(command, check=True, capture_output=True,
                                        text=True)
                assert "MYFLT is deprecated in CS7; use cs_float" in result.stderr
                result = subprocess.run(
                    [*command, "-Werror=deprecated-declarations"],
                    capture_output=True, text=True)
                assert result.returncode != 0, "MYFLT must trigger deprecation"
                source.write_text('''
#include "csdl.h"
void legacy_plugin(cs_float sample) {
    (void)MYFLT2LONG(sample);
    (void)MYFLT2LRND(sample);
    (void)csoundUndenormalizeMYFLT(sample);
}
''')
                subprocess.run(command, check=True, capture_output=True, text=True)
            source.write_text('#include "csound_types.h"\n')
            result = subprocess.run(
                [compiler, "-fsyntax-only", "-I" + args.include_dir,
                 "-DUSE_DOUBLE", "-DUSE_FLOAT", str(source)],
                capture_output=True, text=True)
            assert result.returncode != 0
            assert "cannot be enabled together" in result.stderr
    print("C and C++ numeric types and CS7 compatibility checks passed")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise
