sr=44100
kr=4410
ksmps=10
nchnls=1

giSinusoid      ftgen   0,      0, 8192, 10,    1

pyinit

pyruni {{
import random

pool = [(1 + i/10.0) ** 1.2 for i in range(100)]

def get_number_from_pool(n, p):
    if random.random() < p:
        i = int(random.random() * len(pool))
        pool[i] = n

    return random.choice(pool)
}}

instr 1

        k1      oscil   1, 3, giSinusoid

        k2      pycall1 "get_number_from_pool", k1 + 2, p4

                printk  0.01, k2

endin
