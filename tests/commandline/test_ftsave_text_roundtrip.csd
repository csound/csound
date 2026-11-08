<CsTest>
description = "text table save and load preserve values and oscillator interpolation"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
giRamp ftgen 0, 0, 8, -2, 0, 1, 2, 3, 4, 5, 6, 7
giValues ftgen 0, 0, 8, -2, 1e-20, -1e-20, 1.2345678901234567, -1.2345678901234567, 1e20, -1e20, .00000049, 1.0000000000000002
giRampInit ftgen 0, 0, 8, -2, 0
giValuesInit ftgen 0, 0, 8, -2, 0
giRampControl ftgen 0, 0, 8, -2, 0
giValuesControl ftgen 0, 0, 8, -2, 0
gkChecks init 0

ftsave "test_ftsave_text_init.txt", 1, giRamp, giValues
ftload "test_ftsave_text_init.txt", 1, giRampInit, giValuesInit

instr 1
  iIndex = 0
  while iIndex < 8 do
    iOriginal table iIndex, giValues
    iLoaded table iIndex, giValuesInit
    if iOriginal != iLoaded then
      prints "text save/load changed an init-time table value\n"
      exitnow -1
    endif
    iIndex += 1
  od

  kCycle timeinstk
  kTrigger = (kCycle == 1 ? 1 : 0)
  ftsavek "test_ftsave_text_control.txt", kTrigger, 1, giRamp, giValues
  ftloadk "test_ftsave_text_control.txt", kTrigger, 1, giRampControl, giValuesControl
  kIndex = 0
  while kIndex < 8 do
    kOriginal table kIndex, giValues
    kLoaded table kIndex, giValuesControl
    if kOriginal != kLoaded then
      printks "text save/load changed a control-time table value\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
  ; Halfway between adjacent ramp samples, interpolation must survive loading.
  kOriginal oscili 1, 0, giRamp, .0625
  kInit oscili 1, 0, giRampInit, .0625
  kControl oscili 1, 0, giRampControl, .0625
  if kOriginal != .5 || kInit != kOriginal || kControl != kOriginal then
    printks "text save/load changed oscillator interpolation\n", 0
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 99
  if i(gkChecks) != 1 then
    prints "table round-trip checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .01
i99 .02 .01
</CsScore>
</CsoundSynthesizer>
