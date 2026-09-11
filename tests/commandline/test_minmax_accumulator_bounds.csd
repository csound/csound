<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gaMax init 0
gaMin init 0
gaMaxAbs init 0
gaMinAbs init 0
gkChecks init 0

; Seed each block before the partial notes update the shared accumulators.
instr 1
 gaMax = -2
 gaMin = 2
 gaMaxAbs = .25
 gaMinAbs = 2
endin
instr 2
 aInput = p4
 maxaccum gaMax, aInput
 minaccum gaMin, aInput
 maxabsaccum gaMaxAbs, aInput
 minabsaccum gaMinAbs, aInput
endin
instr 3
 kBase init 0
 kIndex = 0
 while kIndex < ksmps do
  kSample = kBase + kIndex
  kMax = -2
  kMin = 2
  kMaxAbs = .25
  kMinAbs = 2
  if kSample >= 5 && kSample < 15 then
   kMax = 1
   kMin = 1
   kMaxAbs = 1
   kMinAbs = 1
  endif
  if kSample >= 10 && kSample < 45 then
   kMin = -3
   kMaxAbs = 3
  endif
  if kSample >= 64 then
   kMax = -.5
   kMin = -.5
   kMaxAbs = .5
   kMinAbs = .5
  endif
  kActualMax vaget kIndex, gaMax
  kActualMin vaget kIndex, gaMin
  kActualMaxAbs vaget kIndex, gaMaxAbs
  kActualMinAbs vaget kIndex, gaMinAbs
  if kActualMax != kMax || kActualMin != kMin || kActualMaxAbs != kMaxAbs || kActualMinAbs != kMinAbs then
   printks "accumulator mismatch at sample %g: max=%g min=%g maxabs=%g minabs=%g\n", 0, kSample, kActualMax, kActualMin, kActualMaxAbs, kActualMinAbs
   exitnowk(-1)
  endif
  gkChecks += 1
  kIndex += 1
 od
 kBase += ksmps
endin
instr 99
 if i(gkChecks) != 96 then
  prints "accumulator checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01171875
; A note within one block, an overlapping note spanning two blocks,
; and a full-block note. Times are exact multiples of one sample.
i 2 .0006103515625 .001220703125 1
i 2 .001220703125 .0042724609375 -3
i 2 .0078125 .00390625 -.5
i 3 0 .01171875
i 99 .015625 .00390625
e
</CsScore>
</CsoundSynthesizer>
