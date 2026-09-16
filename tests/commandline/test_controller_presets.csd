<CsTest>
description = "Controller preset growth, replacement, array copies, and automatic tags"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
gkDone init 0
ctrlinit 1, 7, 45, 10, 90

instr 1
  kSaved[] ctrlsave 1, 7, 10
  kShort[] fillarray 1, 1, 7, 127
  kStep init 0
  if kStep == 0 then
    kArray ctrlpreset 21, kSaved
    kScalar ctrlpreset 42, 1, 7, 3, 10, 4
    kAuto ctrlpreset 0, 1, 7, 12
    if kArray != 21 || kScalar != 42 || kAuto != 1 then
      exitnowk -1
    endif
    ctrlselect kScalar
    kExpected7 = 3
    kExpected10 = 4
  elseif kStep == 1 then
    ; The saved array has changed with the controllers, but its preset is a copy.
    ctrlselect 21
    kExpected7 = 45
    kExpected10 = 90
  elseif kStep == 2 then
    ; Replace a longer preset with a shorter one.
    kReplacement ctrlpreset 21, kShort
    ctrlselect kReplacement
    kExpected7 = 127
    kExpected10 = 90
  elseif kStep == 3 then
    ; Replace the same slot with a longer preset again.
    kReplacement2 ctrlpreset 21, 1, 7, 0, 10, 127, 11, 64
    ctrlselect kReplacement2
    kExpected7 = 0
    kExpected10 = 127
  elseif kStep == 4 then
    ; Automatically assigned tags use the first free slot after sparse growth.
    kAuto2 ctrlpreset 0, kShort
    if kAuto2 != 2 then
      exitnowk -1
    endif
    ctrlselect 1
    kExpected7 = 12
    kExpected10 = 127
    gkDone = 1
  endif
  kValue7 ctrl7 1, 7, 0, 127
  kValue10 ctrl7 1, 10, 0, 127
  if abs(kValue7 - kExpected7) > .0001 || abs(kValue10 - kExpected10) > .0001 then
    exitnowk -1
  endif
  kStep += 1
endin

instr 2
  if i(gkDone) != 1 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .006
i 2 .01 .001
</CsScore>
</CsoundSynthesizer>
