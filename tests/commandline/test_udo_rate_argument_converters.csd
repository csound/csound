<CsTest>
description = "UDO argument-driven rates prepare audio converters before body initialization"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 64
ksmps = 8
nchnls = 1
0dbfs = 1

opcode ClassicGain, a, ai
  aSignal, iFactor xin
  oversample iFactor, 3, 3
  aResult = aSignal * 2
  xout aResult
endop

opcode ModernGain(aSignal:a, iFactor:i):a
  oversample iFactor, 3, 3
  aSignal *= 2
  xout aSignal
endop

instr CheckConverters
  ; Reinitialize the same UDO calls with a different rate each time.
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

  aInput = 0.25
SET_RATE:
  iFactor = i(kFactor)
  aClassic ClassicGain aInput, iFactor
  aModern ModernGain aInput, iFactor
  rireturn

  ; Both converters have settled by the third block after each init.
  ; The modern UDO must also leave its caller's input unchanged.
  if kCycle % 3 == 0 then
    kClassic downsamp aClassic
    kModern downsamp aModern
    kInput downsamp aInput
    if abs(kClassic - 0.5) > 0.000001 || \
       abs(kModern - 0.5) > 0.000001 || kInput != 0.25 then
      printks "Factor %g expected outputs 0.5 and input 0.25, got %g, %g, %g\n", \
        0, kFactor, kClassic, kModern, kInput
      exitnowk -1
    endif
  endif
endin
</CsInstruments>
<CsScore>
i "CheckConverters" 0 1.5
</CsScore>
</CsoundSynthesizer>
