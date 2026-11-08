<CsTest>
description = "Audio betarand matches scalar draws and preserves inactive samples"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
giReference ftgen 0, 0, 64, -2, 0
gkChecks init 0

instr 1
  seed 1234
  iN = 0
draw:
  iValue betarand 1, p5, p6
  tableiw iValue, p4*32+iN, giReference
  iN += 1
  if iN < 32 igoto draw
endin

instr 2
  seed 1234
  iOffset = int(p2*sr+.5) % ksmps
  kCount init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart+27-kCount)
  aValue betarand 1, p5, p6
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kExpected table p4*32+kCount+kN-kStart, giReference
    endif
    kActual vaget kN, aValue
    if !(abs(kActual-kExpected) < 1e-12) then
      printks "betarand sample %g: expected %g, got %g\n", 0, kN, kExpected, kActual
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd-kStart
  if kCount == 27 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 2 then
    prints "betarand sample checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .001 0 1 1
i1 .01 .001 1 .001 .003
; Start three samples into a block, end two samples before a block boundary.
i2 .1279296875 .0263671875 0 1 1
i2 .2529296875 .0263671875 1 .001 .003
i99 .3 .001
</CsScore>
</CsoundSynthesizer>
