sr=44100
kr=4410
ksmps=10
nchnls=1

pyinit

instr 1

        ktrigger1       metro           3
        ktrigger2       metro           6
        ktrigger3       metro           10

        pyrunt          ktrigger1, "print 'zum'"
        pyrunt          ktrigger2, "print 'pa'"
        pyrunt          ktrigger3, "print 'zi'"

        pylrunt         ktrigger3 + ktrigger1, "print 'zut'"

        a1              rand            5000

                        out             a1

endin
