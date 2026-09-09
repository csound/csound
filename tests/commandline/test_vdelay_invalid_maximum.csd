<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
instr 1
  ain = .25
  ; Reject before converting the requested allocation size to an integer.
  aout vdelay3 ain, .125, p4
  aout2 vdelay ain, .125, p4
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1
i 1 0 .01 1e30
</CsScore>
</CsoundSynthesizer>
