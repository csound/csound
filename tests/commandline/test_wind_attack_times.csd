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

instr 1, 2
  if p1 == 1 then
    aShort wgclar .5, 440, -.3, .01, .1, 0, 0, 0
    aLong wgclar .5, 440, -.3, .3, .1, 0, 0, 0
  else
    aShort wgflute .5, 440, .32, .01, .1, 0, 0, 0
    aLong wgflute .5, 440, .32, .3, .1, 0, 0, 0
  endif
  kTime timeinsts
  kPeak peak aShort
  kDifference peak aShort - aLong
  kChecked init 0
  if kTime >= .1 && kChecked == 0 then
    if !(kPeak > .01 && kDifference > .01) then
      printks "wind attack time has no effect: instrument %g\n", 0, p1
      exitnowk(-1)
    endif
    kChecked = 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1
i 2 0 1
</CsScore>
</CsoundSynthesizer>
