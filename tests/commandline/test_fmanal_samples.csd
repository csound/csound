<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 aPhase phasor p4
 aReal = .1*cos(2*$M_PI*aPhase)
 aImag = .1*sin(2*$M_PI*aPhase)
 aGate = 1
 aAmp, aFreq fmanal aReal, aImag
 ; Cover both inputs reused as either output, including both at once.
 aRealCopy = aReal
 aImagCopy = aImag
 aRealCopy, aImagCopy fmanal aRealCopy, aImagCopy
 aRealSwap = aReal
 aImagSwap = aImag
 aImagSwap, aRealSwap fmanal aRealSwap, aImagSwap
 kStarted init 0
 kCycle init 0
 kN = 0
 while kN < ksmps do
  kGate vaget kN, aGate
  kAmp vaget kN, aAmp
  kFreq vaget kN, aFreq
  kAmpCopy vaget kN, aRealCopy
  kFreqCopy vaget kN, aImagCopy
  kAmpSwap vaget kN, aImagSwap
  kFreqSwap vaget kN, aRealSwap
  kExpectedAmp = 0
  kExpectedFreq = 0
  if kGate != 0 then
   kExpectedAmp = .1
   if kStarted != 0 then
    kExpectedFreq = p4
   endif
   kStarted = 1
  endif
  kAmpError = abs(kAmp-kExpectedAmp) + abs(kAmpCopy-kExpectedAmp) + abs(kAmpSwap-kExpectedAmp)
  kFreqError = abs(kFreq-kExpectedFreq) + abs(kFreqCopy-kExpectedFreq) + abs(kFreqSwap-kExpectedFreq)
  if !(kAmpError < 1e-6) || !(kFreqError < .01) then
   printks "fmanal mismatch: frequency=%g cycle=%g sample=%g amplitude error=%g frequency error=%g\n", 0, p4, kCycle, kN, kAmpError, kFreqError
   exitnowk(-1)
  endif
  kN += 1
 od
 kCycle += 1
 if kCycle == 10 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 4 then
  prints "fmanal checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03 1000
i 1 .04 .03 -1000
i 1 .080625 .02975 1000
i 1 .120625 .02975 -1000
i 99 .16 .01
e
</CsScore>
</CsoundSynthesizer>
