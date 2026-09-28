<CsTest>
description = "Spectral consumers require a live pvbufread in their own instance"
[expect]
exit = "nonzero"
stderr = ["pvinterp: associated pvbufread not found", "pvcross: associated pvbufread not found", "4 errors in performance"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1

instr MakeFile
  aTone oscili .2, 500
  fSpectrum pvsanal aTone, 128, 32, 128, 1
  pvsfwrite fSpectrum, "pvoc.79"
endin

instr Reader
  pvbufread .05, 79
endin

instr Interp
  aOut pvinterp .05, 1, 79, 1, 1, 1, 1, .5, .5
endin

instr Cross
  aOut pvcross .05, 1, "pvoc.79", 1, 0
endin

instr Reused
  if p4 == 1 then
    pvbufread .05, "pvoc.79"
  else
    aOut pvcross .05, 1, "pvoc.79", 1, 0
  endif
endin
</CsInstruments>
<CsScore>
i "MakeFile" 0 .1
; Neither consumer may use another live instrument's reader.
i "Reader" .15 .1
i "Interp" .17 .01
i "Cross" .19 .01
; The old reader has ended.
i "Cross" .3 .01
; A reused instance must not retain its previous reader.
i "Reused" .35 .01 1
i "Reused" .4 .01 0
e
</CsScore>
</CsoundSynthesizer>
