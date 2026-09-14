<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
gkChecks init 0
gkWet init 0

; Unvoiced input must pass through, with silence outside the active samples.
; Prefill outputs so an unwritten tail cannot pass by starting at zero.
instr 1
 aInput = .5
 aTwo init .75
 aThree init .75
 aFour init .75
 aTwo harmon2 aInput, p5, 1, 1, 0, p4, p6
 aThree harmon3 aInput, p5, 1, 1, 1, 0, p4, p6
 aFour harmon4 aInput, p5, 1, 1, 1, 1, 0, p4, p6
 kOffset offsetsmps
 kEarly earlysmps
 kIndex = 0
 while kIndex < ksmps do
  kTwo vaget kIndex, aTwo
  kThree vaget kIndex, aThree
  kFour vaget kIndex, aFour
  kExpected = (kIndex >= kOffset && kIndex < ksmps-kEarly ? .5 : 0)
  if !(abs(kTwo-kExpected)+abs(kThree-kExpected)+abs(kFour-kExpected) < .00001) then
   printks "harmon dry output mismatch: sample=%g expected=%g got=%g %g %g\n", 0, kIndex, kExpected, kTwo, kThree, kFour
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if timeinstk() == 1 then
  gkChecks += 1
 endif
endin

; Disabled extra voices must make all three variants agree. Exercise voiced
; and unvoiced transitions, both frequency modes, and input/output reuse.
instr 2
 kCycle init 0
 kCycle += 1
 aSine oscili .25, 512
 if kCycle % 40 < 8 then
  aInput = 0
 elseif kCycle % 40 < 16 then
  aInput = .25
 else
  aInput = aSine
 endif
 kOct = octcps(512)
 kFirst = (p5 == 0 ? 1 : 512)
 kSecond = (p5 == 0 ? .5 : 256)
 aTwo harmon2 aInput, kOct, kFirst, kSecond, p5, 6, p4
 aThree harmon3 aInput, kOct, kFirst, kSecond, 0, p5, 6, p4
 aFour harmon4 aInput, kOct, kFirst, kSecond, 0, 0, p5, 6, p4
 aAlias = aInput
 aAlias harmon2 aAlias, kOct, kFirst, kSecond, p5, 6, p4
 kOffset offsetsmps
 kEarly earlysmps
 kIndex = 0
 while kIndex < ksmps do
  kInput vaget kIndex, aInput
  kTwo vaget kIndex, aTwo
  kThree vaget kIndex, aThree
  kFour vaget kIndex, aFour
  kAlias vaget kIndex, aAlias
  if !(abs(kTwo) < 4 && abs(kTwo-kThree)+abs(kTwo-kFour)+abs(kTwo-kAlias) < .00001) then
   printks "harmon voice mismatch: cycle=%g sample=%g got=%g %g %g alias=%g\n", 0, kCycle, kIndex, kTwo, kThree, kFour, kAlias
   exitnowk(-1)
  endif
  if (kIndex < kOffset || kIndex >= ksmps-kEarly) && kTwo != 0 then
   printks "harmon wrote an inactive sample\n", 0
   exitnowk(-1)
  endif
  if abs(kTwo) > .001 && abs(kTwo-kInput) > .001 then
   gkWet += 1
  endif
  kIndex += 1
 od
 if kCycle == 1 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 12 || i(gkWet) == 0 then
  prints "harmon checks did not complete: notes=%g wet samples=%g\n", i(gkChecks), i(gkWet)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .25 6 0 1
i 1 .0006103515625 .251220703125 6 9 1
i 1 .002197265625 .00244140625 6 0 -1
; A history longer than 32767 samples and a first pitch estimate of zero.
i 1 0 .25 -1 0 0
i 2 0 .5 1 0
i 2 0 .5 -1 0
i 2 0 .5 0 0
i 2 0 .5 1 1
i 2 .0006103515625 .501220703125 -1 1
i 2 .002197265625 .00244140625 0 1
; Reuse instances with fresh state.
i 1 .625 .25 6 0 1
i 2 .625 .5 1 0
i 99 1.14 .01
e
</CsScore>
</CsoundSynthesizer>
