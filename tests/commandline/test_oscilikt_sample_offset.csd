<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
giRamp ftgen 1, 0, 16, -7, 0, 16, 1
instr 1
  aAmp = 1
  aFreq = 500
  aSync = 0
  aKA oscilikt 1, aFreq, 1, .125
  aAK oscilikt aAmp, 500, 1, .125
  aAA oscilikt aAmp, aFreq, 1, .125
  aSynced oscilikts aAmp, aFreq, 1, aSync, .125
  kSample init 0
  kStart init 3
  kN = kStart
  while kN < ksmps do
    kKA vaget kN, aKA
    kAK vaget kN, aAK
    kAA vaget kN, aAA
    kSynced vaget kN, aSynced
    kExpected = .125 + kSample*500/sr
    if !(abs(kKA-kExpected) + abs(kAK-kExpected) + abs(kAA-kExpected) + abs(kSynced-kExpected) < .00001) then
      printks "oscilikt family read the wrong input sample\n", 0
      exitnowk(-1)
    endif
    kSample += 1
    kN += 1
  od
  kStart = 0
endin
</CsInstruments>
<CsScore>
; Five active samples, starting three samples into the block.
i 1 .000375 .000625
</CsScore>
</CsoundSynthesizer>
