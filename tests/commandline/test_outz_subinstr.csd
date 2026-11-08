<CsTest>
description = "outz returns subinstrument audio to its caller without leaking into the main output"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 2
zakinit 8, 1
0dbfs = 4
gkChecks init 0

instr Child
  setksmps 2
  aLeft = 1
  aRight = 2
  zaw aLeft, 0
  zaw aRight, 1
  outz 0
endin

instr Parent
  aLeft, aRight subinstr "Child"
  kSample = 0
  while kSample < ksmps do
    kLeft vaget kSample, aLeft
    kRight vaget kSample, aRight
    if kLeft != 1 || kRight != 2 then
      printks "subinstr sample %g: expected (1,2), got (%g,%g)\n", \
        0, kSample, kLeft, kRight
      exitnowk -1
    endif
    kSample += 1
  od
  ; Deliberately leave the returned audio out of the main mix.
  gkChecks += 1
endin

instr CheckSilence
  aLeft, aRight monitor
  kSample = 0
  while kSample < ksmps do
    kLeft vaget kSample, aLeft
    kRight vaget kSample, aRight
    if kLeft != 0 || kRight != 0 then
      printks "outz leaked subinstrument audio into the main mix\n", 0
      exitnowk -1
    endif
    kSample += 1
  od
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 2 then
    prints "outz subinstrument checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Parent" 0 .25
i "CheckSilence" 0 .25
i "CheckResults" .25 .25
e
</CsScore>
</CsoundSynthesizer>
