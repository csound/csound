#!/usr/bin/env python3

import subprocess
import os

csound_path = os.getenv("CSOUND_BIN")
opcode6dir64 = os.getenv("OPCODE6DIR64")

env = {
    **os.environ,
    "SSDIR": "include",
    "INCDIR": "include",
    "SADIR": "include",
    "OPCODE6DIR64": opcode6dir64,
}

subprocess.run(
    [csound_path, "-U", "pvanal", "include/flute.aiff", "include/flute.pvx"],
    check=True, env=env
)

subprocess.run(
    [csound_path, "-U", "pvanal", "include/fox.wav", "include/fox.pvx"],
    check=True, env=env
)

subprocess.run(
    [csound_path, "-U", "pvanal", "include/mary.wav", "include/mary.pvx"],
    check=True, env=env
)

subprocess.run(
    [csound_path, "-U", "pvanal", "include/beats.wav", "include/beats.pvx"],
    check=True, env=env
)

subprocess.run(
    [csound_path, "-U", "pvanal", "include/kickroll.wav", "include/kickroll.pvx"],
    check=True, env=env
)

subprocess.run(
    [csound_path, "-U", "hetro", "include/kickroll.wav", "include/kickroll.het"],
    check=True, env=env
)

subprocess.run(
    [csound_path, "-U", "lpanal", "-p34", "include/fox.wav", "include/fox_poles.lpc"],
    check=True, env=env
)

subprocess.run(
    [csound_path, "-U", "lpanal", "-p2", "include/fox.wav", "include/fox_nopoles.lpc"],
    check=True, env=env
)
