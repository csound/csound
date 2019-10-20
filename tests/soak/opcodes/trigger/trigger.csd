<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  kmtr lfo 1, 1, 1			;triangle wave
  kmode = p4
  ktr  trigger kmtr, .5, kmode
  schedkwhen ktr, 0, 3, 2, 0, .3
endin

instr 2
  aenv linseg 0,p3*.1,1,p3*.3,1,p3*.6,0	;envelope
  a1   poscil .3*aenv, 1000, 1
     outs a1, a1
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine

i 1 0 3 0		;down-up
i 1 4 3 2		;down-up & up=down

e
</CsScore>
</CsoundSynthesizer>
