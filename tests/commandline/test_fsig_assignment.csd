<CsTest>
description = "F-signal assignment resumes after gaps and publishes source changes"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkCompleted init 0

instr CopyAfterGap
  kCycle init 0
  kCycle += 1
  ; A new amplitude makes each source frame distinct.
  fSource pvsosc .1*kCycle, 512, 4, 128, 32
  ; p4=0 copies continuously, 1 skips cycle 3, 2 starts at cycle 3.
  kCopy = (p4 == 0 || (p4 == 1 && kCycle != 3) || (p4 == 2 && kCycle >= 3) ? 1 : 0)
  if kCopy == 1 then
    fCopied = fSource
  endif
  kSource[] init 130
  kCopied[] init 130
  kSourceFrame pvs2array kSource, fSource
  kCopiedFrame pvs2array kCopied, fCopied
  if kCopy == 1 && kCopied[16] != kSource[16] then
    printks "Assignment mode %g missed source frame %g\n", 0, p4, kSourceFrame
    exitnowk -1
  endif
  if kCycle == 8 then
    gkCompleted += 1
  endif
endin

instr SwitchSources
  kCycle init 0
  kCycle += 1
  ; Both sources keep the same frame for four control cycles.
  ; Switching must work even when their frame counters are equal.
  fQuiet pvsosc .25, 512, 4, 512, 128
  fLoud pvsosc .5, 512, 4, 512, 128
  if kCycle % 2 == 1 then
    fSelected = fQuiet
    kExpected = .25*1.456
  else
    fSelected = fLoud
    kExpected = .5*1.456
  endif
  ; A consumer must see each selection as a new frame.
  fOutput pvsgain fSelected, 1
  kBins[] init 514
  kFrame pvs2array kBins, fOutput
  if abs(kBins[64] - kExpected) > .000001 then
    printks "Source switch at cycle %g: expected %g, got %g\n", 0, kCycle, kExpected, kBins[64]
    exitnowk -1
  endif
  if kCycle == 8 then
    gkCompleted += 1
  endif
endin

instr RestartSource
  kCycle init 0
  kCycle += 1
  if kCycle == 5 then
    reinit SOURCE
  endif
SOURCE:
  fSource pvsosc .1*kCycle, 512, 4, 128, 32
  rireturn
  fCopied = fSource
  fOutput pvsgain fCopied, 1
  kBins[] init 130
  kFrame pvs2array kBins, fOutput
  if abs(kBins[16] - .1*kCycle*1.456) > .000001 then
    printks "Assignment lost an update after restarting the source, cycle %g\n", 0, kCycle
    exitnowk -1
  endif
  if kCycle == 8 then
    gkCompleted += 1
  endif
endin

instr CheckCompletion
  if i(gkCompleted) != 5 then
    prints "Not all assignment checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CopyAfterGap" 0 .03125 0
i "CopyAfterGap" 0 .03125 1
i "CopyAfterGap" 0 .03125 2
i "SwitchSources" 0 .03125
i "RestartSource" 0 .03125
i "CheckCompletion" .04 .01
e
</CsScore>
</CsoundSynthesizer>
