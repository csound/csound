<CsTest>
description = "cepstrum conversions preserve Nyquist and agree for array and PVS input"

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
gkChecks init 0

instr 1
  iN = p4
  kMagnitudes[] init iN+1
  kFrame[] init 2*(iN+1)
  kReuse[] init iN+1
  kCycle timeinstk
  kIndex = 0
  while kIndex <= iN do
    kMagnitudes[kIndex] = 1+(kIndex+kCycle)/32
    kFrame[2*kIndex] = kMagnitudes[kIndex]
    kFrame[2*kIndex+1] = kIndex*sr/(2*iN)
    kIndex += 1
  od
  kFull[] ceps kMagnitudes, 0
  kCepstrum[] ceps kMagnitudes, p5
  fInput tab2pvs kFrame, ksmps
  kPvsCepstrum[] pvsceps fInput, p5
  kRestored[] cepsinv kCepstrum
  kReuse = kCepstrum
  kReuse cepsinv kReuse
  kIndex = 0
  while kIndex <= iN do
    if abs(kCepstrum[kIndex]-kPvsCepstrum[kIndex]) > .00001 || abs(kRestored[kIndex]-kReuse[kIndex]) > .00001 then
      printks "cepstrum path mismatch at bin %d\n", 0, kIndex
      exitnowk -1
    endif
    if p5 == 0 then
      if abs(kRestored[kIndex]-kMagnitudes[kIndex]) > .00001 then
        printks "cepstrum round trip lost bin %d\n", 0, kIndex
        exitnowk -1
      endif
    else
      kExpected = (kIndex < min(2*p5,iN) ? kFull[kIndex] : 0)
      if abs(kCepstrum[kIndex]-kExpected) > .00001 then
        printks "incorrect coefficient filtering\n", 0
        exitnowk -1
      endif
    endif
    kIndex += 1
  od
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 6 then
    prints "cepstrum checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .1 64 0
i1 0 .1 128 0
i1 0 .1 64 4
i1 0 .1 128 4
i1 0 .1 64 1e30
i1 0 .1 128 1e30
i99 .2 .01
</CsScore>
</CsoundSynthesizer>
