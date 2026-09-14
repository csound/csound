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

instr 1, 2
  setksmps 1
  kHistory[] init 1024
  kPrevious init 0
  kOlder init 0
  kSample init 0
  kWrite init 0
  kL = p4
  kA = 0
  kB = 0
  kD = .8
  kC = 0
  kInput = kSample == 0 ? .1 : 0
  if p5 != 0 then
    kA = .4
    kB = -.2
    kD = .3
    kC = .01
    kInput = .2*sin(kSample*.37)
  endif
  if p5 == 2 then
    kInput *= 40
    kD = 0
  elseif p5 == 3 then
    kL = kSample < 600 ? 1 : (kSample < 1100 ? 20 : 4)
  endif
  aInput = kInput
  if p1 == 1 then
    aOutput nlfilt2 aInput, kA, kB, kD, kC, kL
  else
    aOutput nlfilt aInput, kA, kB, kD, kC, kL
  endif
  kDelay = int(limit(kL, 1, 1024))
  if p1 == 2 then
    ; Preserve the legacy delay offset and clipping rule.
    kDelay += 2
  endif
  kRead = (kWrite-kDelay+2048)%1024
  kState = kA*kPrevious+kB*kOlder+kD*kHistory[kRead]^2-kC+kInput/1.953125
  if p1 == 1 then
    kState = tanh(kState)
  endif
  kExpected = kState*.9765625
  if p1 == 2 then
    if kExpected > 1.953125 then
      kExpected = .9765625
    elseif kExpected < -1.953125 then
      kExpected = -.9765625
    endif
  endif
  kActual downsamp aOutput
  if !(abs(kActual-kExpected) < .00001) then
    printks "nlfilt case %g/%g L=%g sample=%g: got %g, expected %g\n", 0, p1, p5, kL, kSample, kActual, kExpected
    exitnowk(-1)
  endif
  kHistory[kWrite] = kState
  kOlder = kPrevious
  kPrevious = kState
  kWrite = (kWrite+1)%1024
  kSample += 1
  if kSample == 2060 then
    gkChecks += 1
  endif
endin

; Block processing and reused input must match processing one sample at a time.
opcode OneSample, a, a
  setksmps 1
  aInput xin
  aOutput nlfilt2 aInput, .4, -.2, .3, .01, 20
  xout aOutput
endop

instr 3
  aInput oscili .2, 370
  aCopy = aInput
  aRef OneSample aInput
  aOutput nlfilt2 aInput, .4, -.2, .3, .01, 20
  aCopy nlfilt2 aCopy, .4, -.2, .3, .01, 20
  kError max_k abs(aOutput-aRef)+abs(aCopy-aRef), 1, 1
  if !(kError < .00001) then
    printks "nlfilt2 block/input reuse mismatch: %g\n", 0, kError
    exitnowk(-1)
  endif
  kOnce init 1
  if kOnce == 1 then
    gkChecks += 1
    kOnce = 0
  endif
endin

instr 99
  if i(gkChecks) != 16 then
    prints "nonlinear filter checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Delay, signal/parameter pattern. Run past two history-buffer wraps.
i 1 0 .258 1 0
i 1 0 .258 20 0
i 1 0 .258 1024 0
i 1 0 .258 0 0
i 1 0 .258 20.75 0
i 1 0 .258 2000 0
i 1 0 .258 20 1
i 1 0 .258 20 2
i 1 0 .258 1 3
i 2 0 .258 1 0
i 2 0 .258 20 1
i 2 0 .258 20 2
i 2 0 .258 1024 0
i 2 0 .258 2000 0
i 3 .26 .01
i 3 .280625 .009
i 99 .3 .002
</CsScore>
</CsoundSynthesizer>
