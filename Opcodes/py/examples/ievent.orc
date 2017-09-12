sr=44100
kr=4410
ksmps=10
nchnls=2

giSinusoid      ftgen           0,      0, 8192, 10,    1

pyinit

pyruni  {{
import csound
from random import random

for i in range(800):
        csound.ievent(1, i * .2,     0.05, 6.8 + random() * 3,   70.0)
        csound.ievent(1, i * .2,     0.05, 8.8 + random() * 3,   70.0)
}}

instr 1

        iDuration               =       p3
        iFrequency      =       cpsoct(p4)
        iAmplitude      =       ampdb(p5)

        aAmplitude      linseg  iAmplitude, iDuration - 0.01, iAmplitude, 0.002 ,0, 1,0
        aOutput         oscili  aAmplitude, iFrequency, giSinusoid

                        outs    aOutput, aOutput

endin
