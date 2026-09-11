<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode Reference, aaaaaaa, aaak
 setksmps 1
 aInput, aRvt, aDelay, kRvt xin
 kAudioRvt downsamp aRvt
 kDelay downsamp aDelay
 aComb comb aInput, kRvt, 8/sr
 aInv combinv aInput, kRvt, 8/sr
 aAll alpass aInput, kAudioRvt, 8/sr
 aVComb vcomb aInput, kRvt, 8/sr, 32/sr
 aVAll valpass aInput, kRvt, 8/sr, 32/sr
 aVCombAudio vcomb aInput, kRvt, kDelay, 32/sr
 aVAllAudio valpass aInput, kRvt, kDelay, 32/sr
 xout aComb, aInv, aAll, aVComb, aVAll, aVCombAudio, aVAllAudio
endop

instr 1
 kCycle init 0
 kCycle += 1
 kRvt = kCycle < 8 ? .1 : .2
 aInput oscili .05, 437
 aInput += .05
 aMod oscili 1, 173
 aRvt = .1+.03*aMod
 aDelaySamples = 8+4*floor((aMod+1)*.999)
 aDelay = aDelaySamples/sr
 aRefComb, aRefInv, aRefAll, aRefVComb, aRefVAll, aRefVCombAudio, aRefVAllAudio Reference aInput, aRvt, aDelay, kRvt
 aComb comb aInput, kRvt, 8, 0, 1
 aInv combinv aInput, kRvt, 8, 0, 1
 aAll alpass aInput, aRvt, 8, 0, 1
 aAllSeconds alpass aInput, aRvt, 8/sr
 aFixedAll alpass aInput, kRvt, 8, 0, 1
 aRefFixedAll alpass aInput, kRvt, 8/sr
 aVComb vcomb aInput, kRvt, 8, 32, 0, 1
 aVAll valpass aInput, kRvt, 8, 32, 0, 1
 aVCombAudio vcomb aInput, kRvt, aDelaySamples, 32, 0, 1
 aVAllAudio valpass aInput, kRvt, aDelaySamples, 32, 0, 1
 aVCombSeconds vcomb aInput, kRvt, aDelay, 32/sr
 aVAllSeconds valpass aInput, kRvt, aDelay, 32/sr
 aCopy = aInput
 aCopy vcomb aCopy, kRvt, 8, 32, 0, 1
 aCopyAudio = aInput
 aCopyAudio vcomb aCopyAudio, kRvt, aDelaySamples, 32, 0, 1
 aRvtCopy = aRvt
 aRvtCopy alpass aInput, aRvtCopy, 8, 0, 1
 aError = abs(aComb-aRefComb)+abs(aInv-aRefInv)+abs(aAll-aRefAll)+abs(aAllSeconds-aRefAll)
 aError += abs(aFixedAll-aRefFixedAll)
 aError += abs(aVComb-aRefVComb)+abs(aVAll-aRefVAll)+abs(aVCombAudio-aRefVCombAudio)+abs(aVAllAudio-aRefVAllAudio)
 aError += abs(aVCombSeconds-aRefVCombAudio)+abs(aVAllSeconds-aRefVAllAudio)
 aError += abs(aCopy-aRefVComb)+abs(aCopyAudio-aRefVCombAudio)+abs(aRvtCopy-aRefAll)
 kN = 0
 while kN < ksmps do
  kError vaget kN, aError
  if !(kError < .00001) then
   printks "comb family mismatch: cycle=%g sample=%g error=%g\n", 0, kCycle, kN, kError
   exitnowk(-1)
  endif
  kN += 1
 od
 if kCycle == 14 then
  gkChecks += 1
 endif
endin

; Check the decay against the expected impulse response independently of
; the comparison above: after two loops, comb returns input * 0.001^(delay/rvt).
instr 2
 setksmps 1
 kSample init 0
 aInput = kSample == 0 ? .1 : 0
 aComb comb aInput, .1, 8, 0, 1
 kOutput downsamp aComb
 if kSample == 16 then
  iExpected = .1*.001^((8/sr)/.1)
  if !(abs(kOutput-iExpected) < .000001) then
   printks "comb decay mismatch: %g\n", 0, kOutput
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
 kSample += 1
endin

instr 99
 if i(gkChecks) != 3 then
  prints "comb family checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .030029296875
i 1 .0396728515625 .030029296875
i 2 .08 .004
i 99 .1 .002
e
</CsScore>
</CsoundSynthesizer>
