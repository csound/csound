<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
giw ftgen 1, 0, 18, 7, 1, 18, 1
gis ftgen 2, 0, 16, 7, 1, 16, 1
gkcount init 0

instr 1
  aout grain3 100, 0, 0, 0, 0.001, 100, 1, 2, 1, 0, 0, 1234, 0
  ksample downsamp aout
  if gkcount == 0 then
    if ksample < 0.5 then
      printks "grain3 float scheduler did not start a grain: %.9f\\n", 0, ksample
      exitnowk(-1)
    endif
  endif
  gkcount += 1
endin
</CsInstruments>
<CsScore>
i1 0 0.012
</CsScore>
</CsoundSynthesizer>
