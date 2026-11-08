"""Pair native runs of masks 1 and 9. Keep output outside the source tree."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--expressions", type=int, default=512)
    parser.add_argument("--voices", type=int, default=32)
    parser.add_argument("--windows", type=int, default=12)
    parser.add_argument("--unique", action="store_true")
    parser.add_argument("--rate", choices=("i", "k"), default="k")
    parser.add_argument("--pairs", type=int, default=21)
    args = parser.parse_args()
    if args.pairs < 3:
        parser.error("use at least three pairs")
    root = Path(__file__).resolve().parents[2]
    command = [str(args.binary.resolve()), "", str(args.expressions),
               str(args.voices), str(args.windows), str(int(args.unique)), args.rate]
    environment = dict(os.environ, CSOUND7RC=os.devnull)

    def run(mask, capture=None):
        invocation = command.copy()
        invocation[1] = str(mask)
        if capture is not None:
            invocation.append(str(capture))
        result = subprocess.run(invocation, env=environment, check=True,
                                capture_output=True)
        marker, messages = result.stderr.split(b"\n", 1)
        marker = marker.rstrip(b"\r")
        prefix = b"CSOUND_BENCH_LIBRARY "
        if not marker.startswith(prefix):
            raise RuntimeError("native runner did not report its loaded library")
        loaded = Path(os.fsdecode(marker[len(prefix):]))
        if not loaded.samefile(args.library):
            raise RuntimeError(f"loaded {loaded}, expected {args.library}")
        return json.loads(result.stdout), messages

    # Capture every block, including note activation and warmups. Do not use
    # these timings. Native timed runs have no audio file I/O.
    with tempfile.TemporaryDirectory(prefix="csound-cse-bench-") as directory:
        before, after = [Path(directory) / name for name in ("before", "after")]
        first, before_messages = run(1, before)
        second, after_messages = run(9, after)
        audio = before.read_bytes()
        if not any(audio) or audio != after.read_bytes() or before_messages != after_messages:
            raise RuntimeError("before/after audio or messages differ")
        if first["sample_bytes"] != second["sample_bytes"]:
            raise RuntimeError("sample precision differs")
        audio_hash = hashlib.sha256(audio).hexdigest()

    pairs = []
    for pair in range(args.pairs):
        row = {}
        for mask in ((1, 9) if pair % 2 == 0 else (9, 1)):
            row[str(mask)], _ = run(mask)
        pairs.append(row)

    def git(*arguments):
        return subprocess.check_output(["git", *arguments], cwd=root, text=True).strip()

    metadata = {
        "platform": platform.platform(), "cpu": platform.processor(),
        "revision": git("rev-parse", "HEAD"),
        "diff": git("diff", "HEAD"),
        "status": git("status", "--short"),
        "runner_source": Path(__file__).read_text(),
        "binary_sha256": hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        "library_sha256": hashlib.sha256(args.library.read_bytes()).hexdigest(),
        "cmake_cache": (args.build_dir / "CMakeCache.txt").read_text(),
        "benchmark_source": Path(__file__).with_suffix(".c").read_text(),
        "compile_commands": [entry for entry in json.loads(
            (args.build_dir / "compile_commands.json").read_text())
            if Path(entry["file"]).name in ("aops.c", "csound_orc_optimize.c",
                                            "scalar_cse_bench.c")],
    }
    if hasattr(os, "sched_getaffinity"):
        metadata["cpu_affinity"] = sorted(os.sched_getaffinity(0))
    if Path("/proc/cpuinfo").exists():
        metadata["cpuinfo"] = Path("/proc/cpuinfo").read_text()
    elif platform.system() == "Darwin":
        cpu = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"],
                             capture_output=True, text=True)
        metadata["cpu"] = cpu.stdout.strip() if cpu.returncode == 0 else "unavailable"
    summary = {}
    for mask in ("1", "9"):
        summary[mask] = {
            "median_compile_ms": statistics.median(p[mask]["compile_ms"] for p in pairs),
            "median_activation_ms": statistics.median(p[mask]["activation_ms"] for p in pairs),
            "median_cpu_ms_per_window": statistics.median(
                statistics.mean(w["cpu_ms"] for w in p[mask]["windows"]) for p in pairs),
        }
    output = {"metadata": metadata, "settings": {
        "expressions": args.expressions, "unique": args.unique, "rate": args.rate, "voices": args.voices, "windows": args.windows,
        "pairs": args.pairs, "window_score_seconds": 128 * 64 / 48000, "warmup_windows": 3,
        "sample_bytes": first["sample_bytes"], "audio_sha256": audio_hash,
        "messages": before_messages.decode(errors="replace"),
    }, "summary": summary, "pairs": pairs, "complete": True}
    args.output.write_text(json.dumps(output, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
