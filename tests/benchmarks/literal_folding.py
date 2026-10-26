"""Compare literal folding in fresh processes on macOS or Linux.

See optimizer/README.md. No timing threshold is used as a test.
"""
import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import platform
import resource
import subprocess
import sys
import time

from optimizer_bench import Engine, comparison, compile_dump, library

HEADER = "sr=48000\nksmps=64\nnchnls=1\n0dbfs=1\nseed 12345\n"

def source(case):
    if case == "musical":
        return """
instr 1
 iAmp = (0.2+0.1)*0.25
 iRatio = 3*0.5
 iRise = 0.01+0.02
 aEnv linseg 0, iRise, iAmp, 1, 0
 aSignal oscili aEnv, p4*iRatio
 out aSignal
endin
"""
    count = 512 if case == "literal512" else 64
    lines = ["instr 1"]
    for i in range(count):
        left = "p4" if case == "dynamic64" else "0.125"
        lines.append(f"iValue{i} = ({left}+{i}.25)*0.5")
    lines.append("iSum = " + "+".join(f"iValue{i}" for i in range(count)))
    lines += ["aSignal = iSum*0.000001", "out aSignal", "endin"]
    return "\n".join(lines) + "\n"


def peak_rss():
    value = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    return value if sys.platform == "darwin" else value * 1024


def worker(args):
    lib = library(args.library)
    engine = Engine(lib, args.passes)
    code = (HEADER + source(args.case)).encode()
    result = {}
    transcript = []

    def measure(name, action):
        cpu = time.process_time_ns()
        wall = time.perf_counter_ns()
        status = action()
        result[name + "_wall_ms"] = (time.perf_counter_ns() - wall) / 1e6
        result[name + "_cpu_ms"] = (time.process_time_ns() - cpu) / 1e6
        engine.check(status)
        transcript.append(engine.messages())
        result[name + "_peak_rss_bytes"] = peak_rss()

    try:
        measure("compile", lambda: lib.csoundCompileOrc(engine.cs, code, 0))
        measure("start", lambda: lib.csoundStart(engine.cs))
        events = "\n".join(f"i 1 0 2 {220+i}" for i in range(args.voices))
        lib.csoundEventString(engine.cs, events.encode(), 0)
        measure("activation", lambda: lib.csoundPerformKsmps(engine.cs))
        samples = bytearray()
        sample_bytes = 64 * lib.csoundGetSizeOfMYFLT()
        if args.capture:
            samples.extend(C.string_at(lib.csoundGetSpout(engine.cs), sample_bytes))

        def render():
            for _ in range(750):  # One second of steady blocks, after activation.
                status = lib.csoundPerformKsmps(engine.cs)
                if status:
                    return status
                if args.capture:
                    samples.extend(C.string_at(lib.csoundGetSpout(engine.cs), sample_bytes))
            return 0

        measure("render", render)
        result["sample_bytes"] = lib.csoundGetSizeOfMYFLT()
        result["messages"] = transcript
        if args.capture:
            result["audio_sha256"] = hashlib.sha256(samples).hexdigest()
        return result
    finally:
        engine.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--repeats", type=int, default=21)
    parser.add_argument("--warmups", type=int, default=3)
    parser.add_argument("--timeout", type=float, default=300,
                        help="maximum seconds per worker (default: 300)")
    parser.add_argument("--voices", type=int, default=128)
    parser.add_argument("--case", choices=("musical", "dynamic64", "literal64", "literal512"))
    parser.add_argument("--passes", type=int, choices=(1, 3), default=1)
    parser.add_argument("--worker", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--capture", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    if (args.voices < 1 or args.voices > 1024 or args.repeats < 1 or args.warmups < 0
            or not 0 < args.timeout < float("inf")):
        parser.error("invalid counts or worker timeout")
    if args.worker:
        if not args.case:
            parser.error("worker needs a case")
        print(json.dumps(worker(args)))
        return
    if not args.output or not args.build_dir:
        parser.error("--output and --build-dir are required")

    def run(case, passes, capture=False):
        command = [sys.executable, str(Path(__file__).resolve()), "--worker",
                   "--library", str(args.library.resolve()), "--case", case,
                   "--passes", str(passes), "--voices", str(args.voices)]
        if capture:
            command.append("--capture")
        try:
            return json.loads(subprocess.check_output(
                command, text=True, timeout=args.timeout))
        except subprocess.TimeoutExpired:
            parser.error(f"{case} worker with pass mask {passes} exceeded "
                         f"{args.timeout:g} seconds")

    root = Path(__file__).resolve().parents[2]
    def git(*arguments):
        try:
            return subprocess.check_output(
                ["git", "-C", str(root), *arguments], text=True, timeout=30).strip()
        except subprocess.TimeoutExpired:
            parser.error("git metadata command exceeded 30 seconds")

    report = {"complete": False, "metadata": {
        "command": sys.argv, "platform": platform.platform(),
        "python": sys.version, "revision": git("rev-parse", "HEAD"),
        "diff": git("diff", "HEAD"), "sample_rate": 48000, "ksmps": 64,
        "voices": args.voices, "repeats": args.repeats, "warmups": args.warmups,
        "worker_timeout_seconds": args.timeout,
        "library_sha256": hashlib.sha256(args.library.read_bytes()).hexdigest(),
        "cmake_cache": (args.build_dir / "CMakeCache.txt").read_text(),
        "compile_commands": [e for e in json.loads(
            (args.build_dir / "compile_commands.json").read_text())
            if e["file"].endswith(("/OOps/aops.c", "/Engine/csound_orc_optimize.c"))],
        "environment": {k: os.environ.get(k) for k in
                        ("CSOUND7RC", "OPCODE7DIR64", "OPCODE7DIR")},
        "affinity": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
    }, "results": []}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    lib = library(args.library)
    for case in ([args.case] if args.case else ["musical", "dynamic64", "literal64", "literal512"]):
        print(case, file=sys.stderr)
        # Capture and dumps never contribute timing samples.
        captures = [run(case, p, True) for p in (1, 3)]
        if (captures[0]["audio_sha256"] != captures[1]["audio_sha256"] or
                captures[0]["messages"] != captures[1]["messages"]):
            raise RuntimeError(f"output mismatch: {case}")
        dumps = [compile_dump(lib, source(case), 64, p) for p in (1, 3)]
        pairs = []
        for repeat in range(args.warmups + args.repeats):
            pair = [None, None]
            for mode in ((0, 1) if repeat % 2 == 0 else (1, 0)):
                pair[mode] = run(case, (1, 3)[mode])
            if repeat >= args.warmups:
                pairs.append(pair)
        metrics = [stage + suffix for stage in ("compile", "start", "activation", "render")
                   for suffix in ("_cpu_ms", "_wall_ms", "_peak_rss_bytes")]
        report["results"].append({"case": case, "source": source(case),
            "dumps": dumps, "verification": captures, "pairs": pairs,
            "summary": {m: comparison(pairs, m) for m in metrics}})
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    report["complete"] = True
    args.output.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
