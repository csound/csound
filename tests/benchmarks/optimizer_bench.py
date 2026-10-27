"""Paired optimizer baseline: compile, start, activation block and steady render.

Standard library only. See optimizer/README.md for semantics and limitations.
"""
import argparse
import ctypes as C
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import re
import statistics
import subprocess
import sys
import time

DEBUG_OPTIMIZER = 0x200


def optimizer_options(passes):
    # Keep numeric labels in the benchmark data, but use the public named options.
    names = {1: "legacy", 2: "constant-folding", 4: "copy-removal",
             8: "common-subexpression-elimination"}
    if passes < 0 or passes & ~sum(names):
        raise ValueError(f"unsupported optimizer mask {passes}")
    return ["--optimize=none"] + [
        f"--optimize-{name}" for bit, name in names.items() if passes & bit]


class Engine:
    def __init__(self, lib, passes, dump=False):
        options = optimizer_options(passes)
        self.lib = lib
        self.cs = lib.csoundCreate(None, b"")
        if not self.cs:
            raise RuntimeError("csoundCreate failed")
        lib.csoundCreateMessageBuffer(self.cs, 0)
        for option in ["-n -d -m4"] + options + (
                [f"--verbose={DEBUG_OPTIMIZER}"] if dump else ["--verbose=0"]):
            self.check(lib.csoundSetOption(self.cs, option.encode()))
        controls = lib.csoundQueryGlobalVariable(self.cs, b"__orc_optimizer")
        if (not controls or controls[0] != passes or
                bool(lib.csoundGetDebug(self.cs) & DEBUG_OPTIMIZER) != dump):
            self.close()
            raise RuntimeError("optimizer controls were not applied")

    def messages(self):
        parts = []
        while self.lib.csoundGetMessageCnt(self.cs):
            parts.append(self.lib.csoundGetFirstMessage(self.cs).decode(errors="replace"))
            self.lib.csoundPopFirstMessage(self.cs)
        return "".join(parts)

    def check(self, status):
        if status:
            raise RuntimeError(f"Csound status {status}: {self.messages()}")

    def close(self):
        self.lib.csoundDestroy(self.cs)


def library(path):
    lib = C.CDLL(str(path.resolve()))
    lib.csoundGetSizeOfMYFLT.restype = C.c_int
    scalar = {4: C.c_float, 8: C.c_double}[lib.csoundGetSizeOfMYFLT()]
    signatures = {
        "csoundGetDebug": ([C.c_void_p], C.c_int32),
        "csoundQueryGlobalVariable": ([C.c_void_p, C.c_char_p], C.POINTER(C.c_int32)),
        "csoundCreate": ([C.c_void_p, C.c_char_p], C.c_void_p),
        "csoundDestroy": ([C.c_void_p], None),
        "csoundCreateMessageBuffer": ([C.c_void_p, C.c_int], None),
        "csoundGetMessageCnt": ([C.c_void_p], C.c_int),
        "csoundGetFirstMessage": ([C.c_void_p], C.c_char_p),
        "csoundPopFirstMessage": ([C.c_void_p], None),
        "csoundSetOption": ([C.c_void_p, C.c_char_p], C.c_int),
        "csoundCompileOrc": ([C.c_void_p, C.c_char_p, C.c_int], C.c_int),
        "csoundStart": ([C.c_void_p], C.c_int),
        "csoundEventString": ([C.c_void_p, C.c_char_p, C.c_int], None),
        "csoundPerformKsmps": ([C.c_void_p], C.c_int),
        "csoundGetSpout": ([C.c_void_p], C.POINTER(scalar)),
    }
    for name, (parameters, result) in signatures.items():
        getattr(lib, name).argtypes = parameters
        getattr(lib, name).restype = result
    return lib


def trial(lib, body, block, voices, seconds, passes, capture=False, dump=False,
          block_times=False, live_compile=False):
    engine = Engine(lib, passes, dump)
    try:
        source = (f"sr=48000\nksmps={block}\nnchnls=1\n0dbfs=1\nseed 12345\n" + body).encode()
        start = time.perf_counter_ns()
        engine.check(lib.csoundCompileOrc(engine.cs, source, 0))
        compile_ms = (time.perf_counter_ns() - start) / 1e6
        start = time.perf_counter_ns()
        engine.check(lib.csoundStart(engine.cs))
        start_ms = (time.perf_counter_ns() - start) / 1e6
        # Long notes avoid note termination in the timed interval.
        events = "\n".join(f"i 1 0 {seconds+10} {220+i}" for i in range(voices))
        lib.csoundEventString(engine.cs, events.encode(), 0)
        start = time.perf_counter_ns()
        engine.check(lib.csoundPerformKsmps(engine.cs))
        activation_ms = (time.perf_counter_ns() - start) / 1e6
        blocks = math.ceil(seconds * 48000 / block)
        audio = bytearray()
        if capture:
            audio.extend(C.string_at(lib.csoundGetSpout(engine.cs),
                                     block * lib.csoundGetSizeOfMYFLT()))
        timings = []
        start = time.perf_counter_ns()
        for _ in range(blocks):
            if block_times:
                tick = time.perf_counter_ns()
            engine.check(lib.csoundPerformKsmps(engine.cs))
            if block_times:
                timings.append(time.perf_counter_ns() - tick)
            if capture:
                audio.extend(C.string_at(lib.csoundGetSpout(engine.cs),
                                         block * lib.csoundGetSizeOfMYFLT()))
        render_ms = (time.perf_counter_ns() - start) / 1e6
        result = dict(compile_ms=compile_ms, start_ms=start_ms,
                      activation_block_ms=activation_ms, render_ms=render_ms,
                      samples_per_second=blocks * block / (render_ms / 1000))
        if live_compile:
            start = time.perf_counter_ns()
            engine.check(lib.csoundCompileOrc(engine.cs,
                b"instr 999\nkx = (2+3)*4\nendin\n", 0))
            result["live_compile_ms"] = (time.perf_counter_ns() - start) / 1e6
        if capture:
            result["audio_sha256"] = hashlib.sha256(audio).hexdigest()
        if block_times:
            ordered = sorted(timings)
            result["block_ns"] = timings
            result["block_summary_ns"] = {
                f"p{q}": ordered[min(len(ordered)-1, math.ceil(q/100*len(ordered))-1)]
                for q in (50, 95, 99)}
            result["block_summary_ns"].update(max=max(ordered),
                deadline_exceedances=sum(t > block / 48000 * 1e9 for t in timings))
        return result, engine.messages()
    finally:
        engine.close()


def comparison(pairs, metric):
    ratios = [p[1][metric] / p[0][metric] for p in pairs]
    rng = random.Random(12345)
    bootstrap = sorted(statistics.median(rng.choices(ratios, k=len(ratios)))
                       for _ in range(2000))
    return {"off_median": statistics.median(p[0][metric] for p in pairs),
            "on_median": statistics.median(p[1][metric] for p in pairs),
            "on_over_off_median": statistics.median(ratios),
            "paired_bootstrap_95pct": [bootstrap[50], bootstrap[1949]]}


def compile_dump(lib, body, block, passes):
    engine = Engine(lib, passes, dump=True)
    try:
        source = (f"sr=48000\nksmps={block}\nnchnls=1\n0dbfs=1\nseed 12345\n" + body).encode()
        engine.check(lib.csoundCompileOrc(engine.cs, source, 0))
        # Other existing verbose messages may contain unstable raw TREE details.
        return "".join(re.findall(r"ORC-OPT .*?ORC-OPT end\n", engine.messages(), flags=re.S))
    finally:
        engine.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cases", nargs="*", help="fixture stems; default all")
    parser.add_argument("--ksmps", nargs="+", type=int, default=[1, 16, 64, 256])
    parser.add_argument("--voices", nargs="+", type=int, default=[1, 32])
    parser.add_argument("--seconds", type=float, default=0.25)
    parser.add_argument("--repeats", type=int, default=21)
    parser.add_argument("--warmups", type=int, default=3)
    parser.add_argument("--live-compile", action="store_true",
                        help="separate live compilation experiment")
    parser.add_argument("--block-times", action="store_true",
                        help="separate block timing experiment; at least 10000 blocks")
    args = parser.parse_args()
    if (min(args.ksmps + args.voices) < 1 or args.seconds <= 0 or
            not math.isfinite(args.seconds) or args.repeats < 1 or args.warmups < 0):
        parser.error("positive counts and finite seconds required; warmups >= 0")
    if args.block_times and any(math.ceil(args.seconds*48000/k) < 10000 for k in args.ksmps):
        parser.error("block timing requires at least 10000 blocks per trial")
    fixtures = Path(__file__).parent / "optimizer"
    files = sorted(fixtures.glob("*.orc"))
    if args.cases:
        known = {f.stem for f in files}
        if set(args.cases) - known:
            parser.error(f"unknown cases: {set(args.cases)-known}")
        files = [f for f in files if f.stem in args.cases]
    lib = library(args.library)
    root = Path(__file__).resolve().parents[2]
    git = lambda *a: subprocess.check_output(["git", "-C", str(root), *a], text=True).strip()
    metadata = dict(command=sys.argv, platform=platform.platform(), cpu=platform.processor(),
                    python=sys.version, revision=git("rev-parse", "HEAD"),
                    git_status=git("status", "--short"),
                    library=str(args.library.resolve()),
                    library_sha256=hashlib.sha256(args.library.read_bytes()).hexdigest(),
                    float_bytes=lib.csoundGetSizeOfMYFLT(),
                    compile_commands=[entry for entry in json.loads((args.build_dir / "compile_commands.json").read_text())
                                      if entry["file"].endswith("/OOps/aops.c")],
                    compiler_info=[f.read_text() for f in args.build_dir.glob("CMakeFiles/*/CMakeCCompiler.cmake")],
                    cmake_cache=(args.build_dir / "CMakeCache.txt").read_text(),
                    seed=12345, sample_rate=48000, warmups=args.warmups,
                    repeats=args.repeats, seconds=args.seconds, block_times=args.block_times,
                    live_compile=args.live_compile,
                    environment={k: os.environ.get(k) for k in ("OPCODE7DIR64", "OPCODE7DIR", "CSOUND_HOME", "CSOUND7RC")})
    results = []
    for file in files:
        body = file.read_text()
        for block in args.ksmps:
            for voices in args.voices:
                print(f"{file.stem}: ksmps={block} voices={voices}", file=sys.stderr)
                # Correctness/dumps are outside all measured trials.
                verification = [trial(lib, body, block, voices, args.seconds, p,
                                      capture=True, live_compile=args.live_compile) for p in (0, 1)]
                if verification[0][0]["audio_sha256"] != verification[1][0]["audio_sha256"]:
                    raise RuntimeError(f"differential audio mismatch: {file}")
                if verification[0][1] != verification[1][1]:
                    raise RuntimeError(f"differential diagnostics mismatch: {file}")
                dumps = [compile_dump(lib, body, block, p) for p in (0, 1)]
                pairs = []
                for repeat in range(args.warmups + args.repeats):
                    pair = [None, None]
                    for mode in ((0, 1) if repeat % 2 == 0 else (1, 0)):
                        pair[mode], _ = trial(lib, body, block, voices, args.seconds, mode,
                                             block_times=args.block_times, live_compile=args.live_compile)
                    if repeat >= args.warmups:
                        pairs.append(pair)
                results.append(dict(case=file.stem, source=body, ksmps=block, voices=voices,
                    verification_sha256=verification[0][0]["audio_sha256"],
                    dumps=dumps, pairs=pairs,
                    summary={m: comparison(pairs, m) for m in
                             (["compile_ms", "start_ms", "activation_block_ms", "render_ms"] +
                              (["live_compile_ms"] if args.live_compile else []))}))
                args.output.write_text(json.dumps(dict(complete=False, metadata=metadata, results=results), indent=2)+"\n")

    args.output.write_text(json.dumps(dict(complete=True, metadata=metadata, results=results), indent=2)+"\n")


if __name__ == "__main__":
    main()
