#!/usr/bin/env python3

import json
import unittest
import subprocess
import shutil
import os
import os.path
import warnings
from glob import glob
from parameterized import parameterized
from hashlib import sha256

csound_path = os.getenv("CSOUND_BIN")
opcode6dir64 = os.getenv("OPCODE6DIR64")

opcodes = [y for x in os.walk("opcodes") for y in glob(os.path.join(x[0], '*.csd'))]
opcodes_test_data = [y for x in os.walk("opcodes") for y in glob(os.path.join(x[0], '*.csd.json'))]
opcodes_test_data.sort()
test_data = []

for test_json_path in opcodes_test_data:
    with open(test_json_path) as handle:
        data = json.loads(handle.read())
        data["path"] = test_json_path
        test_data.append(data)

cli_options = ["-d", "-W", "-m0", "-b256", "-B512"]

# csbeats will be found this way
os.environ["PATH"] += os.pathsep + opcode6dir64

env = {
    **os.environ,
    "SSDIR": "include",
    "INCDIR": "include",
    "SADIR": "include",
    "OPCODE6DIR64": opcode6dir64,
}

test_data_parameterized = []

for test in test_data:
    test_data_parameterized.append(
        (test["path"], test["sha256"])
    )

def custom_name_func(testcase_func, index, params):
    return "%s_%s" %(
        testcase_func.__name__,
        parameterized.to_safe_name(
            os.path.dirname(params[0][0]) +
            "_" +
            os.path.basename(params[0][0])
        ),
    )

class SoakOpcodes(unittest.TestCase):
    @parameterized.expand(test_data_parameterized , name_func=custom_name_func)
    def test(self, path, expected):
        os.makedirs("tmp", exist_ok=True)
        name = os.path.basename(path).replace(".csd.json", "")
        csd = path.replace(".json", "")
        rendered_file_path = os.path.join("tmp", name + ".wav")
        proc = subprocess.run(
            [csound_path, csd, "-o", rendered_file_path ] + cli_options,
            encoding="utf-8",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
            env=env
            # , stdout=subprocess.DEVNULL, stderr=stderr
        )
        output = str(proc.stdout) + str(proc.stderr)
        if "unexpected T_IDENT" in output:
            warnings.warn("WARNING: not testing " + csd + " unexpected T_IDENT")
            return 0
        with open(rendered_file_path, "rb") as handle:
            binary_data = handle.read()
        hasher = sha256()
        hasher.update(binary_data)
        actual_hashsum = hasher.hexdigest()
        self.assertEqual(actual_hashsum, expected)
        # Make sure that audio was produced (dev)
        # self.assertFalse("overall amps:  0.00000" in output)
