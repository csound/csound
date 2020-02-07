<CsoundSynthesizer>

<CsInstruments>
sr = 44100
ksmps = 16
nchnls = 1
0dbfs = 1

giBuzz ftgen 5,0,4096,11,20,1,1
giWFn ftgen 7,0,16384,20,2,1
instr 1
aSig grain3 200, 0, 1, 0.01, 0.5, 80, 300, giBuzz, giWFn, 0, 0
out aSig*0.06
endin
</CsInstruments>

<CsScore>

i 1 0 30
e
</CsScore>

</CsoundSynthesizer>
