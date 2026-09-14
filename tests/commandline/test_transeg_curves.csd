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

opcode Curve, k, kk
 kPosition, kType xin
 if abs(kType) < 1e-6 then
  kValue = kPosition
 elseif kType > 50 then
  kValue = (exp(kType*(kPosition-1))-exp(-kType))/(1-exp(-kType))
 else
  kValue = (1-exp(kType*kPosition))/(1-exp(kType))
 endif
 xout kValue
endop

instr 1
 setksmps 1
 kEnv transeg 0, .015625, p4, 1, .015625, -p4, 0
 aEnv transeg 0, .015625, p4, 1, .015625, -p4, 0
 kBreak transegb 0, .015625, p4, 1, .03125, -p4, 0
 aBreak transegb 0, .015625, p4, 1, .03125, -p4, 0
 kAudio downsamp aEnv
 kAudioBreak downsamp aBreak
 kSample init 0
 if kSample < 125 then
  kExpected Curve kSample/125, p4
 elseif kSample < 250 then
  kFall Curve (kSample-125)/125, -p4
  kExpected = 1-kFall
 else
  kExpected = 0
 endif
 kError = abs(kEnv-kExpected)+abs(kAudio-kExpected)+abs(kBreak-kExpected)+abs(kAudioBreak-kExpected)
 if !(kError < .00003) then
  printks "transeg curve=%g sample=%g expected=%g control=%g audio=%g breakpoint=%g/%g\n", 0, p4, kSample, kExpected, kEnv, kAudio, kBreak, kAudioBreak
  exitnowk(-1)
 endif
 if kSample == 260 then
  gkChecks += 1
 endif
 kSample += 1
endin

instr 2
 setksmps 1
 kEnv transegr 0, .03125, p4, 1, .015625, p5, 0
 aEnv transegr 0, .03125, p4, 1, .015625, p5, 0
 kAudio downsamp aEnv
 kSample init 0
 kReleaseSample init 0
 kReleasing release
 if kReleasing != 0 then
  if kReleaseSample == 0 then
   kStart Curve kSample/250, p4
  endif
  kFall Curve kReleaseSample/125, p5
  kExpected = kStart*(1-kFall)
  kReleaseSample += 1
 else
  kExpected Curve kSample/250, p4
 endif
 kError = abs(kEnv-kExpected)+abs(kAudio-kExpected)
 if !(kError < .00003) then
  printks "transegr curve=%g release curve=%g sample=%g expected=%g control=%g audio=%g\n", 0, p4, p5, kSample, kExpected, kEnv, kAudio
  exitnowk(-1)
 endif
 if kSample == 240 then
  gkChecks += 1
 endif
 kSample += 1
endin

opcode AudioReference, a, i
 setksmps 1
 iCurve xin
 aEnv transeg 0, .015625, iCurve, 1, .015625, -iCurve, 0
 xout aEnv
endop

instr 3
 aEnv transeg 0, .015625, p4, 1, .015625, -p4, 0
 aBreak transegb 0, .015625, p4, 1, .03125, -p4, 0
 aRef AudioReference p4
 aError = abs(aEnv-aRef)+abs(aBreak-aRef)
 kN = 0
 while kN < ksmps do
  kError vaget kN, aError
  if !(kError < .00003) then
   printks "transeg partial block differs: curve=%g sample=%g error=%g\n", 0, p4, kN, kError
   exitnowk(-1)
  endif
  kN += 1
 od
 kCycle init 0
 kCycle += 1
 if kCycle == 16 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 23 then
  prints "transeg checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .034 0
i 1 0 .034 1e-8
i 1 0 .034 -1e-8
i 1 0 .034 1e-20
i 1 0 .034 -1e-20
i 1 0 .034 2
i 1 0 .034 -2
i 1 0 .034 1000
i 1 0 .034 -1000
i 2 .04 .015625 0 0
i 2 .04 .015625 1e-8 1e-8
i 2 .04 .015625 -1e-8 -1e-8
i 2 .04 .015625 1e-20 1e-20
i 2 .04 .015625 -1e-20 -1e-20
i 2 .04 .015625 2 2
i 2 .04 .015625 -2 -2
i 2 .04 .015625 1000 1000
i 2 .04 .015625 -1000 -1000
i 2 .04 .015625 2 0
i 3 .100625 .034 2
i 3 .100625 .034 -2
i 3 .100625 .034 1e-20
i 3 .100625 .034 1000
i 99 .16 .002
e
</CsScore>
</CsoundSynthesizer>
