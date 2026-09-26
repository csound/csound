<CsTest>
description = "undersample maps note boundaries to local samples and clears the caller's inactive samples"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

gkCalls init 0

opcode HalfRate, a, 0
  undersample 2, 4
  gkCalls += 1
  aOne = 1
  xout aOne
endop

instr Check
  gkCalls = 0
  aOutput HalfRate
  if gkCalls != 1 then
    exitnowk -1
  endif
  ; Note samples 3..6 map to local samples 2..3. The local block is
  ; [0, 0, 1, 1]. Linear conversion gives [0, 0, 0, 0, 0, .5, 1, 1],
  ; then the caller's final inactive sample must be cleared.
  kExpected[] fillarray 0, 0, 0, 0, 0, 0.5, 1, 0
  kSample = 0
  while kSample < ksmps do
    kActual vaget kSample, aOutput
    if kActual != kExpected[kSample] then
      printks "sample=%g: expected=%g, got=%g\n", \
              0, kSample, kExpected[kSample], kActual
      exitnowk -1
    endif
    kSample += 1
  od
endin
</CsInstruments>
<CsScore>
; Start at sample 3 and play four samples, all within one caller block.
i "Check" 0.09375 0.125
e
</CsScore>
</CsoundSynthesizer>
