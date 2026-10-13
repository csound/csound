<CsTest>
description = "tvconv aligns audio-rate freeze controls with partial blocks"

[expect]
exit = 0
</CsTest>
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

instr CheckFreeze
 ; The note starts five samples into a block. Audio signals are zero before
 ; that offset. Reading aWrite from sample zero would freeze the wrong samples.
 aInput = .25
 aCoefficients = .125
 aWrite = 1
 aReference tvconv aInput, aCoefficients, 1, 1, p4, 8
 aFreezeInput tvconv aInput, aCoefficients, aWrite, 1, p4, 8
 aFreezeCoefficients tvconv aInput, aCoefficients, 1, aWrite, p4, 8
 kCycle init 0
 kCycle += 1
 kSample = 0
 while kSample < ksmps do
  kReference vaget kSample, aReference
  kInput vaget kSample, aFreezeInput
  kCoefficients vaget kSample, aFreezeCoefficients
  if !(abs(kReference-kInput) + abs(kReference-kCoefficients) < 1e-6) then
   printks "tvconv freeze offset: partition=%g cycle=%g sample=%g reference=%g input=%g coefficients=%g\n", 0, p4, kCycle, kSample, kReference, kInput, kCoefficients
   exitnowk(-1)
  endif
  kSample += 1
 od
 if kCycle == 16 then
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 3 then
  prints "tvconv freeze checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckFreeze" [5/8192] .0405 1
i "CheckFreeze" [5/8192] .0405 2
i "CheckFreeze" [5/8192] .0405 8
i "CheckResults" .05 .01
e
</CsScore>
</CsoundSynthesizer>
