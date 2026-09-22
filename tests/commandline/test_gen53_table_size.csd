<CsTest>
description = "GEN53 initializes lookup fields for deferred and resized tables"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
giSource ftgen 1, 0, 8, -2, 1, 0, 0, 0, 0, 0, 0, 0

instr 1
  iResult ftgen 0, 0, p4, -53, giSource, 2
  iChannels ftchnls iResult
  if ftlen(iResult) != 8 || iChannels != 1 then
    prints "GEN53 returned the wrong table size or channel count\n"
    exitnow(-1)
  endif
  ; The reconstructed impulse lies at phase .5 of the eight-point table.
  aValue oscili 1, 0, iResult, .5
  kValue downsamp aValue
  if !(abs(kValue - 1) < .00001) then
    printks "GEN53 lookup used the wrong table size\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 4
</CsScore>
</CsoundSynthesizer>
