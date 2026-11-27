#!/usr/bin/env python3

# The metadata record of a Wasm plugin must survive its link: read it from the
# final module with --list-plugins, which reads the file without loading it.

import argparse
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csound-executable", required=True)
    parser.add_argument("--plugin", required=True)
    args = parser.parse_args()

    plugin = Path(args.plugin).resolve()
    environment = dict(os.environ)
    # list only the plugin, not the plugin directories of this machine
    environment["OPCODE7DIR64"] = str(plugin.parent / "no-such-plugin-dir")
    environment["CS_USER_PLUGINDIR"] = str(plugin.parent / "no-such-user-dir")
    result = subprocess.run(
        [args.csound_executable, "--list-plugins", f"--opcode-lib={plugin}"],
        env=environment,
        capture_output=True,
        text=True,
        timeout=30,
    )
    output = result.stdout + result.stderr
    if result.returncode != 0:
        raise RuntimeError(
            f"Csound exited with {result.returncode}\n{output}")
    rows = [line.split() for line in output.splitlines()]
    if [plugin.name, "0.1.0-wasm", str(plugin)] not in rows:
        raise RuntimeError(f"{plugin.name} is not listed with its version\n"
                           f"{output}")


if __name__ == "__main__":
    main()
