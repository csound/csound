<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

icount init 0
loop:
  inum = icount / 3
  inm  = int(inum)
  prints "integer (%f/3) = %f\\n", icount, inm
loop_lt icount, 1, 10, loop

endin
</CsInstruments>
<CsScore>

i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
