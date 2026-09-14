<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 120
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode TrackSample, aa, ak
 setksmps 1
 aInput, kGain xin
 aFreq, aLock plltrack aInput, kGain
 xout aFreq, aLock
endop

; Changing the block size must not change the samples that reach the PLL.
; Include block-constant square waves, silence, DC, and ordinary audio.
instr 1
 kCycle init 0
 kCycle += 1
 aSine oscili .5, 200
 if kCycle <= 400 then
  aInput = (kCycle % 2 == 0 ? .5 : -.5)
 elseif kCycle <= 800 || kCycle > 1600 then
  aInput = 0
 elseif kCycle <= 1200 then
  aInput = aSine
 else
  aInput = .5
 endif
 kGain = (kCycle <= 800 ? .1 : .15)
 aFreq, aLock plltrack aInput, kGain
 aRefFreq, aRefLock TrackSample aInput, kGain
 aAlias = aInput
 aAlias, aAliasLock plltrack aAlias, kGain
 kIndex = 0
 while kIndex < ksmps do
  kFreq vaget kIndex, aFreq
  kLock vaget kIndex, aLock
  kRefFreq vaget kIndex, aRefFreq
  kRefLock vaget kIndex, aRefLock
  kAlias vaget kIndex, aAlias
  kAliasLock vaget kIndex, aAliasLock
  if !(abs(kFreq-kRefFreq)+abs(kLock-kRefLock)+abs(kFreq-kAlias)+abs(kLock-kAliasLock) < .00001) then
   printks "plltrack block mismatch: cycle=%g sample=%g freq=%g ref=%g alias=%g lock=%g ref=%g alias=%g\n", 0, kCycle, kIndex, kFreq, kRefFreq, kAlias, kLock, kRefLock, kAliasLock
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if kCycle == 400 then
  if !(kFreq > 190 && kFreq < 210) then
   printks "plltrack failed to track a 200 Hz square: %g\n", 0, kFreq
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
 if kCycle == 800 || kCycle == 2000 then
  if !(abs(kFreq)+abs(kLock) < .001) then
   printks "plltrack retained output during silence: freq=%g lock=%g\n", 0, kFreq, kLock
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
 if kCycle == 1 then
  gkChecks += 1
 endif
endin
instr 99
 if i(gkChecks) != 6 then
  prints "plltrack checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 5
; Partial first and last blocks, including a note contained in one block.
i 1 .000625 .03125
i 1 .00125 .000625
i 99 5.01 .01
e
</CsScore>
</CsoundSynthesizer>
