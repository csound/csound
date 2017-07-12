<CsoundSynthesizer>
<CsOptions>
-o test.wav -d
</CsOptions>
<CsInstruments>
sr     = 44100
kr     = 4410
ksmps  = 10
nchnls = 2
0dbfs=1
i1 ftgen 1,0,2048,10,1
instr 1 ;Simple sine at 440Hz
a1	oscili 0.5,440, 1
outs a1, a1
endin

</CsInstruments>
<CsScore>

i1 0 30
</CsScore>
</CsoundSynthesizer>
