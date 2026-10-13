<CsTest>
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
gkFirst[] init 1440

instr 1
  aSig wgbow .5, 440, 3, .127, 6, .1
  kBlock init 0
  if kBlock < 45 then
    kI = 0
    while kI < ksmps do
      kSample vaget kI, aSig
      kIndex = kBlock*ksmps + kI
      if p4 == 0 then
        gkFirst[kIndex] = kSample
      elseif !(abs(kSample-gkFirst[kIndex]) < .000001) then
        printks "wgbow reused a previous note's state\n", 0
        exitnowk(-1)
      endif
      kI += 1
    od
    kBlock += 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .05 0
i 1 .1 .05 1
</CsScore>
</CsoundSynthesizer>
