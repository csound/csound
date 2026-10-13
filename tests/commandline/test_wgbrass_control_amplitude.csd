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

instr 1
  kAmp = .5
  aConstant wgbrass .5, 440, .5, .1, 0, 0
  aControl wgbrass kAmp, 440, .5, .1, 0, 0
  kTime timeinsts
  kPeak peak aConstant
  kDifference peak aConstant-aControl
  kChecked init 0
  if kTime > .3 && kChecked == 0 then
    if !(kPeak > .001 && kDifference < .000001) then
      printks "wgbrass k-rate amplitude differs from constant amplitude\n", 0
      exitnowk(-1)
    endif
    kChecked = 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .5
</CsScore>
</CsoundSynthesizer>
