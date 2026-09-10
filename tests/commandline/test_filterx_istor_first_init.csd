<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  asig = .001
  if p4 == 1 then
    aout tonex asig, 1000, 4, 1
  elseif p4 == 2 then
    aout atonex asig, 1000, 4, 1
  elseif p4 == 3 then
    aout resonx asig, 1000, 100, 4, 0, 1
  elseif p4 == 4 then
    ksig = .001
    kout resonxk ksig, 1000, 100, 4, 0, 1
    aout = kout
  else
    aout resony asig, 1000, 100, 4, 0, 0, 0, 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 1
i 1 .02 .01 2
i 1 .04 .01 3
i 1 .06 .01 4
i 1 .08 .01 5
</CsScore>
</CsoundSynthesizer>
