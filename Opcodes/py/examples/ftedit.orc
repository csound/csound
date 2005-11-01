sr=44100
kr=4410
ksmps=10
nchnls=1

pyinit

pyruni {{
from music.gui import ftedit

table = csound.ftable(10, 8192, 10, 1)

ftedit.edit(table)

csound.ievent(1, 0, 5, 8.0, 60, table)
}}

instr 1

        iDuration       =       p3
        iFrequency      =       cpsoct(p4)
        iAmplitude      =       ampdb(p5)
        iTable          =       p6

        kAmplitude      linen   iAmplitude, 1, iDuration, 1
        aSignal         oscil   kAmplitude, iFrequency, iTable

                        out     aSignal

endin
