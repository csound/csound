sr=44100
kr=4410
ksmps=10
nchnls=1

giSinusoid      ftgen   0,      0, 8192, 10,    1

pyinit

pyruni {{
import random

class GetNumberFromPool:
    def __init__(self, e=1.2, a=1.0, f=0.1, N=100):
        self.pool = [(a + i*f) ** e for i in range(N)]

    def __call__(self, n, p):
        if random.random() < p:
            i = int(random.random() * len(self.pool))
            self.pool[i] = n

        return random.choice(self.pool)

get_number_from_pool = GetNumberFromPool()
}}

instr 1

        k1      oscil   1, 3, giSinusoid

        k2      pycall1 "get_number_from_pool", k1 + 2, p4

                printk  0.01, k2

endin
