"""Measure expression-argument parsing or compilation in a built Csound library.

Example (use identical build flags and run before/after in separate processes):
  python3 tests/benchmarks/compile_expression_args.py --library /path/to/libcsound.so

Construction, common engine setup, tree deletion, and destruction are untimed.
This synthetic workload measures compile latency, not audio performance.
Nesting fillarray in lenarray prevents statement rewriting from bypassing
expression-argument expansion.
"""
import argparse
import ctypes
import json
from pathlib import Path
import statistics
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--phase", choices=("parse", "compile"), default="compile")
    parser.add_argument("--kind", choices=("literals", "expressions", "nested"),
                        default="expressions")
    parser.add_argument("--arguments", type=int, nargs="+", default=[8, 64, 256, 1024])
    parser.add_argument("--instruments", type=int, default=8)
    parser.add_argument("--repeats", type=int, default=21)
    parser.add_argument("--warmups", type=int, default=3)
    args = parser.parse_args()
    if (not all(0 < count < 1999 for count in args.arguments) or
            args.instruments < 1 or args.repeats < 1 or args.warmups < 0):
        parser.error("counts must be positive, arguments < 1999, warmups >= 0")

    lib = ctypes.CDLL(str(args.library.resolve()))
    signatures = {
        "csoundCreate": ([ctypes.c_void_p, ctypes.c_char_p], ctypes.c_void_p),
        "csoundDestroy": ([ctypes.c_void_p], None),
        "csoundSetOption": ([ctypes.c_void_p, ctypes.c_char_p], ctypes.c_int),
        "csoundCreateMessageBuffer": ([ctypes.c_void_p, ctypes.c_int], None),
        "csoundGetFirstMessage": ([ctypes.c_void_p], ctypes.c_char_p),
        "csoundDestroyMessageBuffer": ([ctypes.c_void_p], None),
        "csoundCompileOrc": ([ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int], ctypes.c_int),
        "csoundParseOrc": ([ctypes.c_void_p, ctypes.c_char_p], ctypes.c_void_p),
        "csoundDeleteTree": ([ctypes.c_void_p, ctypes.c_void_p], None),
    }
    for name, (parameters, result) in signatures.items():
        function = getattr(lib, name)
        function.argtypes, function.restype = parameters, result

    results = []
    for count in args.arguments:
        values = []
        for index in range(count):
            value = str(index) if args.kind == "literals" else f"p4 + {index}"
            if args.kind == "nested":
                value = f"abs({value})"
            values.append(value)
        expression = ", ".join(values)
        source = "\n".join(
            f"instr {number + 1}\niCount = lenarray(fillarray({expression}))\nendin"
            for number in range(args.instruments)).encode()
        samples = []
        for iteration in range(args.warmups + args.repeats):
            csound = lib.csoundCreate(None, b"")
            if not csound:
                raise RuntimeError("csoundCreate failed")
            lib.csoundCreateMessageBuffer(csound, 0)
            try:
                if lib.csoundSetOption(csound, b"-n -d -m0") != 0:
                    raise RuntimeError("csoundSetOption failed")
                if lib.csoundCompileOrc(csound, b"sr=48000\nksmps=64\nnchnls=1\n0dbfs=1", 0):
                    raise RuntimeError("engine setup failed")
                start = time.perf_counter_ns()
                if args.phase == "parse":
                    tree = lib.csoundParseOrc(csound, source)
                    elapsed = time.perf_counter_ns() - start
                    ok = bool(tree)
                    if tree:
                        lib.csoundDeleteTree(csound, tree)
                else:
                    status = lib.csoundCompileOrc(csound, source, 0)
                    elapsed = time.perf_counter_ns() - start
                    ok = status == 0
                if not ok:
                    raise RuntimeError(lib.csoundGetFirstMessage(csound))
                if iteration >= args.warmups:
                    samples.append(elapsed / 1e6)
            finally:
                lib.csoundDestroyMessageBuffer(csound)
                lib.csoundDestroy(csound)
        results.append({"arguments": count, "median_ms": statistics.median(samples),
                        "samples_ms": samples})
    print(json.dumps({"library": str(args.library.resolve()), "phase": args.phase,
                      "kind": args.kind, "instruments": args.instruments,
                      "repeats": args.repeats, "results": results}, indent=2))


if __name__ == "__main__":
    main()
