<CsTest>
description = "poscil preserves scalar phase across block sizes, table wraps and partial blocks"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
giPowerOfTwo ftgen 1, 0, 1024, 10, 1
giOtherSize ftgen 2, 0, -1000, 10, 1
gkFinished init 0

opcode OneSample, a, kki
  setksmps 1
  kAmp, kFrequency, iTable xin
  aSignal poscil kAmp, kFrequency, iTable, .1234567
  xout aSignal
endop

instr CompareBlocks
  iTable = p4
  kFrequency init p5
  aReference OneSample .2, kFrequency, iTable
  aSignal poscil .2, kFrequency, iTable, .1234567
  kError maxk aSignal - aReference, 1, 1
  if !(kError < .000001) then
    printks "poscil table=%g frequency=%g error=%g\n", 0, iTable, kFrequency, kError
    exitnowk -1
  endif
  kBlocks init 0
  if kBlocks == 1000 then
    gkFinished += 1
  endif
  kBlocks += 1
endin

instr CheckCompletion
  if i(gkFinished) != 6 then
    prints "Not all poscil phase checks completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Non-aligned starts and durations exercise inactive samples and short tails.
; Include forward, backward, and more than one table wrap per sample.
i "CompareBlocks" .0000625 2.000104167 1 440
i "CompareBlocks" .0000625 2.000104167 1 -440
i "CompareBlocks" .0000625 2.000104167 1 144440
i "CompareBlocks" .0000625 2.000104167 2 440
i "CompareBlocks" .0000625 2.000104167 2 -440
i "CompareBlocks" .0000625 2.000104167 2 -144440
i "CheckCompletion" 2.01 .01
e
</CsScore>
</CsoundSynthesizer>
