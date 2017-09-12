sr=44100
kr=4410
ksmps=10
nchnls=1

        pyinit

        pyruni "import random"

        pyexeci "pyexec1.py"

instr 1

        pyexec          "pyexec2.py"

        pylexeci        "pyexec3.py"
        pylexec         "pyexec4.py"

endin
