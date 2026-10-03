#!/usr/bin/env python3

# This test reads Wasmtime's private cache layout. CI pins Wasmtime 47.0.2 in
# .github/workflows/csound_builds.yml. Recheck module_cache_files() when the pin
# changes.

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def run_csound(
    csound,
    csd,
    plugin,
    opcode_dir,
    environment,
    should_succeed=True,
    extra_options=(),
):
    command = [
        csound,
        f"--opcode-lib={plugin}",
        *extra_options,
        str(csd),
    ]
    if opcode_dir:
        environment["OPCODE7DIR64"] = opcode_dir

    result = subprocess.run(
        command,
        cwd=plugin.parent,
        env=environment,
        capture_output=True,
        text=True,
        timeout=30,
    )
    if should_succeed and result.returncode != 0:
        raise RuntimeError(
            f"Csound exited with {result.returncode}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    if not should_succeed and result.returncode == 0:
        raise RuntimeError(
            "Csound accepted a missing explicit Wasm opcode library\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return result


def module_cache_files(cache_dir):
    modules = cache_dir / "modules"
    if not modules.is_dir():
        return []
    return sorted(
        path
        for path in modules.rglob("*")
        if path.is_file()
        and ".wip-" not in path.name
        and path.suffix != ".stats"
        and path.stat().st_size > 0
    )


def add_cache_variant_section(module):
    name = b"csound.cache.variant"
    payload = bytes([len(name)]) + name + b"\x01"
    if len(payload) >= 128:
        raise RuntimeError("cache variant section is too large")
    return module + b"\x00" + bytes([len(payload)]) + payload


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csound-executable", required=True)
    parser.add_argument("--opcode7dir64")
    parser.add_argument("--plugin", required=True)
    parser.add_argument("--source-dir", required=True)
    args = parser.parse_args()

    source_dir = Path(args.source_dir).resolve()
    plugin = Path(args.plugin).resolve()
    csd = source_dir / "test_wasmtime_opcode_cache.csd"

    with tempfile.TemporaryDirectory(prefix="csound-wasmtime-cache-") as root:
        root = Path(root)
        home_dir = root / "home"
        config_dir = root / "config"
        cache_home = root / "cache-home"
        cache_dir = cache_home / "wasmtime"
        override_cache_dir = cache_home / "wasmtime-override"
        plugin_variant = root / "velvetlp-variant.wasm"
        plain_csd = root / "plain.csd"
        retry_csd = root / "retry.csd"
        config = config_dir / "wasmtime-cache.toml"
        override_config = config_dir / "wasmtime-cache-override.toml"

        home_dir.mkdir()
        config_dir.mkdir()
        cache_home.mkdir()
        plugin_bytes = plugin.read_bytes()
        config.write_text(
            f'[cache]\ndirectory = "{cache_dir.as_posix()}"\n',
            encoding="utf-8",
        )
        override_config.write_text(
            f'[cache]\ndirectory = "{override_cache_dir.as_posix()}"\n',
            encoding="utf-8",
        )
        plain_csd.write_text(
            """<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.001
</CsScore>
</CsoundSynthesizer>
""",
            encoding="utf-8",
        )
        retry_csd.write_text(
            """<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --num-threads=1
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
  aRetry wasmretryonce
  out aRetry
endin
instr 2
  iCalls wasmretrycount
  prints "WASM_RETRY_COUNT=%.0f\\n", iCalls
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.012 0.001
</CsScore>
</CsoundSynthesizer>
""",
            encoding="utf-8",
        )

        environment = os.environ.copy()
        environment.update(
            {
                "HOME": str(home_dir),
                "XDG_CONFIG_HOME": str(config_dir),
                "XDG_CACHE_HOME": str(cache_home),
                "CSOUND_WASMTIME_CACHE_CONFIG": str(config),
            }
        )

        run_csound(
            args.csound_executable,
            plain_csd,
            root / "missing.wasm",
            args.opcode7dir64,
            environment,
            should_succeed=False,
        )

        run_csound(
            args.csound_executable,
            csd,
            plugin,
            args.opcode7dir64,
            environment,
            extra_options=(
                "--env:CSOUND_WASMTIME_CACHE_CONFIG="
                f"{override_config}",
            ),
        )

        if not module_cache_files(override_cache_dir):
            raise RuntimeError("Csound --env cache override was not used")
        if module_cache_files(cache_dir):
            raise RuntimeError("process cache config overrode Csound --env")

        retry_result = run_csound(
            args.csound_executable,
            retry_csd,
            plugin,
            args.opcode7dir64,
            environment,
        )
        retry_output = retry_result.stdout + retry_result.stderr
        retry_match = re.search(r"WASM_RETRY_COUNT=([0-9]+)", retry_output)
        if retry_match is None or int(retry_match.group(1)) < 2:
            raise RuntimeError(
                "Wasm opcode did not run again after returning NOTOK\n"
                f"stdout:\n{retry_result.stdout}\n"
                f"stderr:\n{retry_result.stderr}"
            )
        if (
            "Wasm opcode failed" in retry_output
            or "Wasm opcode trapped" in retry_output
        ):
            raise RuntimeError(
                "Guest NOTOK was reported as a Wasmtime failure\n"
                f"stdout:\n{retry_result.stdout}\n"
                f"stderr:\n{retry_result.stderr}"
            )

        run_csound(
            args.csound_executable,
            csd,
            plugin,
            args.opcode7dir64,
            environment,
        )

        artifacts = module_cache_files(cache_dir)
        if not artifacts:
            raise RuntimeError(f"no Wasmtime module cache file found in {cache_dir}")

        first_artifacts = set(artifacts)
        plugin_variant.write_bytes(add_cache_variant_section(plugin_bytes))
        run_csound(
            args.csound_executable,
            csd,
            plugin_variant,
            args.opcode7dir64,
            environment,
        )
        variant_artifacts = set(module_cache_files(cache_dir))
        if not variant_artifacts > first_artifacts:
            raise RuntimeError("changed raw Wasm did not create a new cache key")

        artifact = artifacts[0]
        corrupt_bytes = b"corrupt Wasmtime module cache entry"
        artifact.write_bytes(corrupt_bytes)

        run_csound(
            args.csound_executable,
            csd,
            plugin,
            args.opcode7dir64,
            environment,
        )

        if artifact.read_bytes() == corrupt_bytes:
            raise RuntimeError("Wasmtime did not replace the corrupt cache entry")


if __name__ == "__main__":
    main()
