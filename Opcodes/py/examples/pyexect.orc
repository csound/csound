sr=44100
kr=4410
ksmps=10
nchnls=1

pyinit

instr 1

        ktrigger1       metro           3
        ktrigger2       metro           6
        ktrigger3       metro           10
        ktrigger4       metro           0.5

        pyexect         ktrigger1, "pyexect1.py"
        pyexect         ktrigger2, "pyexect2.py"
        pyexect         ktrigger3, "pyexect3.py"

        pylexect        ktrigger3 + ktrigger1, "pyexect4.py"

        a1              rand            5000

                        out             a1

endin
