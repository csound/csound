<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
giEight ftgen 1, 0, 8, -7, 0, 8, 1
giSix ftgen 2, 0, -6, -7, 0, 6, 1
giTen ftgen 3, 0, -10, -7, 0, 10, 1
instr 1
  kCycle timeinstk
  kTable = (kCycle < 3 ? p4 : (kCycle < 5 ? 2 : (kCycle < 7 ? 1 : 3)))
  aAmp = 1
  aFreq = 500
  aSync = 0
  kOut oscilikt 1, 500, kTable, .125
  aKK oscilikt 1, 500, kTable, .125
  aKA oscilikt 1, aFreq, kTable, .125
  aAK oscilikt aAmp, 500, kTable, .125
  aAA oscilikt aAmp, aFreq, kTable, .125
  aPhase osciliktp 500, kTable, .125
  aSynced oscilikts 1, 500, kTable, aSync, .125
  kKK downsamp aKK
  kKA downsamp aKA
  kAK downsamp aAK
  kAA downsamp aAA
  kPhase downsamp aPhase
  kSynced downsamp aSynced
  kExpected = .125 + (kCycle-1)*500/sr
  if !(abs(kOut-kExpected) + abs(kKK-kExpected) + abs(kKA-kExpected) + abs(kAK-kExpected) + abs(kAA-kExpected) + abs(kPhase-kExpected) + abs(kSynced-kExpected) < .00001) then
    printks "oscilikt family lost phase at cycle %d, table %d\n", 0, kCycle, kTable
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Start with integer phase, then with floating phase.
i 1 0 .001 1
i 1 .002 .001 2
</CsScore>
</CsoundSynthesizer>
