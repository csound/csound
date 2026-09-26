<CsTest>
description = "UDO audio-array rate conversion matches scalar signals across reinit"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 128
ksmps = 8
nchnls = 1

opcode ArrayDown, a[], a[]i
  aInput[], iFactor xin
  undersample iFactor, 4
  aResult[] = aInput + aInput
  xout aResult
endop

opcode ScalarDown, a, ai
  aInput, iFactor xin
  undersample iFactor, 4
  xout aInput + aInput
endop

opcode ArrayUp, a[], a[]i
  aInput[], iFactor xin
  oversample iFactor, 4, 4
  aResult[] = aInput + aInput
  xout aResult
endop

opcode ScalarUp, a, ai
  aInput, iFactor xin
  oversample iFactor, 4, 4
  xout aInput + aInput
endop

instr CheckRates
  kCycle timeinstk
  kFactor init 2
  if kCycle == 4 then
    kFactor = 4
    reinit SET_RATE
  elseif kCycle == 7 then
    kFactor = 1
    reinit SET_RATE
  elseif kCycle == 10 then
    kFactor = 2
    reinit SET_RATE
  endif
  aRamp phasor 1
  aInput[] init 2
  aInput[0] = aRamp
  aInput[1] = .25 + aRamp
SET_RATE:
  if p4 == 0 then
    aOutput[] ArrayDown aInput, i(kFactor)
    aFirst ScalarDown aInput[0], i(kFactor)
    aSecond ScalarDown aInput[1], i(kFactor)
  else
    aOutput[] ArrayUp aInput, i(kFactor)
    aFirst ScalarUp aInput[0], i(kFactor)
    aSecond ScalarUp aInput[1], i(kFactor)
  endif
  rireturn

  ; Each array element must match an independent scalar converter,
  ; including its filter history and every sample after reinitialization.
  kSample = 0
  while kSample < ksmps do
    kFirst vaget kSample, aFirst
    kSecond vaget kSample, aSecond
    kArrayFirst vaget kSample, aOutput[0]
    kArraySecond vaget kSample, aOutput[1]
    if abs(kArrayFirst-kFirst) > .000001 || abs(kArraySecond-kSecond) > .000001 then
      printks "direction %g, factor %g, sample %g expected [%g, %g], got [%g, %g]\n", \
        0, p4, kFactor, kSample, kFirst, kSecond, kArrayFirst, kArraySecond
      exitnowk -1
    endif
    kSample += 1
  od
endin
</CsInstruments>
<CsScore>
i "CheckRates" 0 .75 0
i "CheckRates" 1 .75 1
</CsScore>
</CsoundSynthesizer>
