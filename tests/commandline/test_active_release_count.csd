<CsTest>
description = "active excludes releasing notes from total current counts only"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
giChecks init 0
gkChecks init 0

instr 10
  xtratim .04
endin

instr Voice
  xtratim .04
endin

instr 1
  kAll active 0
  kAllNoRelease active 0, 0, 1
  kNumeric active 10
  kNumericNoRelease active 10, 0, 1
  kNamed active "Voice"
  kNamedNoRelease active "Voice", 0, 1
  kReleased = kNumeric - kNumericNoRelease + kNamed - kNamedNoRelease
  if kAll-kAllNoRelease != kReleased then
    printks "active: total excluded %g release notes, expected %g\n", 0, kAll-kAllNoRelease, kReleased
    exitnowk -1
  endif

  kTotalEver active 0, 1
  kTotalEverNoRelease active 0, 1, 1
  kNumericEver active 10, 1
  kNumericEverNoRelease active 10, 1, 1
  kNamedEver active "Voice", 1
  kNamedEverNoRelease active "Voice", 1, 1
  if kTotalEver != kTotalEverNoRelease || kNumericEver != kNumericEverNoRelease || kNamedEver != kNamedEverNoRelease then
    printks "active: release exclusion changed historical counts\n", 0
    exitnowk -1
  endif
  gkChecks += 1
endin

instr 90
  iAll active 0
  iAllNoRelease active 0, 0, 1
  iNumeric active 10
  iNumericNoRelease active 10, 0, 1
  iNamed active "Voice"
  iNamedNoRelease active "Voice", 0, 1
  if iAll-iAllNoRelease != p4 || iNumeric-iNumericNoRelease != p4/2 || iNamed-iNamedNoRelease != p4/2 then
    prints "active: wrong init-time release count at %g\n", p2
    exitnow -1
  endif
  iNumericEver active 10, 1
  iNumericEverNoRelease active 10, 1, 1
  iNamedEver active "Voice", 1
  iNamedEverNoRelease active "Voice", 1, 1
  if iNumericEver != iNumericEverNoRelease || iNamedEver != iNamedEverNoRelease then
    prints "active: wrong init-time historical count\n"
    exitnow -1
  endif
  giChecks += 1
endin

instr 99
  if giChecks != 3 || i(gkChecks) == 0 then
    prints "active release count checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i10 0 .01
i10 0 .1
i"Voice" 0 .01
i1 .001 .08
i90 .002 .001 0
i90 .02 .001 2
i90 .07 .001 0
i99 .2 .001
</CsScore>
</CsoundSynthesizer>
