<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1

instr 1
  setksmps 4
  Sreceived invalue "text"
  Scopy strcpyk Sreceived
  ; The host checks the copy after changing its callback's text.
  chnset Scopy, "copy"
endin
</CsInstruments>
<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
