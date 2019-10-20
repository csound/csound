<CsoundSynthesizer>
<CsInstruments>

; Example written by Joachim Heintz 07/2008

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

; non-normalized function table with a sample 1
giFile	ftgen 1, 0, 0, -1, "fox.wav", 0, 0, 0

; Instrument #1 - uses poscil3 for playing samples from a function table
instr 1
kamp = p4
kspeed	= p5
ifn = 1
iskip = p6
kcps	= kspeed / (ftlen(ifn) / ftsr(ifn)); frequency of the oscillator
iphs	= iskip / (ftlen(ifn) / ftsr(ifn)); calculates skiptime to phase values (0-1)
  
  a1 poscil3 kamp, kcps, ifn, iphs
  out a1
endin
</CsInstruments>
<CsScore>
i1 0 2.756 1 1 0
i1 3 2.756 1 -1 0
i1 6 1.378 1 .5 2.067
e
</CsScore>
</CsoundSynthesizer>
