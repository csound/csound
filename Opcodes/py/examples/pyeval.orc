sr=44100
kr=4410
ksmps=10
nchnls=1

        pyinit

        pyruni  "from random import random"

instr 1

                pyrun   "a = random()"
        k1      pyeval  "a"
                printk  0.1, k1
endin
