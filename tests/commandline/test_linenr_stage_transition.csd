<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iOffset = int(p2*sr + .5) % ksmps
  iAttack = int(p4)
  iAttackK = int(p4/ksmps + .5)
  iSamples = int(p3*sr + .5) + int(p5*kr + .5)*ksmps
  kCount init 0
  kBlock init 0
  kReleaseSamples init 0
  kReleaseBlocks init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = (p5 > 0 ? ksmps : min(ksmps, kStart + iSamples - kCount))
  aSignal init 0
  kN = 0
  while kN < ksmps do
    vaset .5 + (kCount + kN - kStart)/256, kN, aSignal
    kN += 1
  od
  if p6 == 0 then
    aOut linenr 1, p4/sr, p5, .25
  elseif p6 == 1 then
    aOut linenr aSignal, p4/sr, p5, .25
  else
    aSignal linenr aSignal, p4/sr, p5, .25
    aOut = aSignal
  endif
  kOut linenr 1, p4/sr, p5, .25
  if p5 > 0 then
    kReleasing release
  else
    kReleasing = 0
  endif
  kExpectedK = 1
  if kBlock < iAttackK then
    kExpectedK = kBlock/iAttackK
  endif
  if kReleasing == 1 && p5 > 0 then
    kExpectedK *= .25^(kReleaseBlocks/(p5*kr))
    kReleaseBlocks += 1
  endif
  if !(abs(kOut - kExpectedK) < .00001) then
    printks "FAIL control linenr block=%g: %g expected %g\n", 0, kBlock, kOut, kExpectedK
    exitnowk -1
  endif
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aOut
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kExpected = 1
      if kSample < iAttack then
        kExpected = kSample/iAttack
      endif
      if kReleasing == 1 && p5 > 0 then
        kExpected *= .25^(kReleaseSamples/(p5*sr))
        kReleaseSamples += 1
      endif
      if p6 != 0 then
        kExpected *= .5 + kSample/256
      endif
    endif
    if !(abs(kActual - kExpected) < .00001) then
      printks "FAIL linenr attack=%g decay=%g mode=%g offset=%g sample=%g release=%g: %g expected %g\n", \
          0, p4, p5, p6, iOffset, kCount + kN - kStart, kReleasing, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kBlock += 1
  kCount += kEnd - kStart
  if kBlock == 3 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 14 then
    prints "FAIL linenr checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Attack ends inside the block: control input, audio input, and input reuse.
i 1 0 .0625 4 0 0
i 1 .1279296875 .0625 4 0 1
i 1 .2529296875 .0625 4 0 2
; Release after the sustain stage.
i 1 .375 .0625 4 .03125 0
i 1 .5 .0625 4 .03125 1
i 1 .625 .0625 4 .03125 2
; Release overlaps the end of attack. Keep the last attack factor.
i 1 .75 .015625 40 .0625 0
i 1 .875 .015625 40 .0625 1
i 1 1 .015625 40 .0625 2
; Zero attack and attack endings on either side of a block boundary.
i 1 1.125 .0625 0 .03125 0
i 1 1.25 .0625 0 .03125 1
i 1 1.375 .0625 0 .03125 2
i 1 1.5 .0625 16 0 2
i 1 1.625 .0625 17 0 1
i 99 1.875 .015625
</CsScore>
</CsoundSynthesizer>
