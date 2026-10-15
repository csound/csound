#!/usr/bin/env python3
"""Run the opt-in WASI thread checks with a host-side deadline."""

import argparse
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wasmtime", default="wasmtime")
    parser.add_argument("--timeout", type=float, default=45)
    parser.add_argument("--build-dir", type=Path, required=True)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    test_dir = args.build_dir.resolve() / "platform/wasm-wasi/tests/realtime"
    cases = (
        ("wasi-pthread-smoke", [], 0, "WASI pthread smoke passed"),
        ("wasi-realtime-smoke", [], 0, "WASI Csound realtime smoke passed"),
        ("wasi-realtime-smoke", ["--fatal-init"], 1, "WASI fatal init reached"),
        ("wasi-realtime-smoke", ["--fatal-thread"], 1, "WASI fatal init reached"),
    )
    for name, arguments, expected_status, marker in cases:
        module = test_dir / name
        command = [
            args.wasmtime, "run", "-Sthreads=y", "-Wthreads=y",
            "-Wshared-memory=y", "-Wexceptions=y", "-Ccache=n", str(module),
        ] + arguments
        try:
            result = subprocess.run(command, capture_output=True, text=True,
                                    timeout=args.timeout, check=False)
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"{module}: {error}", file=sys.stderr)
            return 1
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        # A Wasmtime trap also exits with status 1. It is not the deliberate
        # process exit required for a fatal error on a Csound-created worker.
        host_error = any(text in result.stderr.lower() for text in
                         ("error while executing", "wasm trap", "caused by:"))
        if (result.returncode != expected_status or host_error or
                marker not in result.stdout.splitlines()):
            print(f"{module}: smoke failed (status {result.returncode})",
                  file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
