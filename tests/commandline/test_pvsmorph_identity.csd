<CsTest>
description = "pvsmorph preserves identical ordinary and sliding spectra"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  ; Every blend of a spectrum with itself must preserve its centroid.
  aTone oscili .5, 1024
  fSource pvsanal aTone, 64, p4, 64, 1
  kBlend line 0, p3, 1
  fMorphed pvsmorph fSource, fSource, kBlend, 1-kBlend
  kSource pvscent fSource
  kMorphed pvscent fMorphed
  kCycle timeinstk
  ; Wait for the analysis window to fill.
  if kCycle == 100 then
    if !(abs(kSource-1024) < .01 && abs(kMorphed-kSource) < .01) then
      printks "hop %g: source centroid %g, morphed centroid %g\n", 0, p4, kSource, kMorphed
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
endin

instr 2
  if gkChecks != 2 then
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Hop 16 uses ordinary frames; hop 1 uses sliding analysis.
i1 0 .25 16
i1 .3 .25 1
i2 .6 .01
e
</CsScore>
</CsoundSynthesizer>
