<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
giorder init 0

instr 1
  korder init 1
  if timeinstk() == 2 then
    korder = 8
    reinit FILTER
  endif
  kgoto PLAY

FILTER:
  iorder = i(korder)
  giorder = iorder
  istor = iorder == 1 ? 0 : 1
  asig = .001
  aout resonx asig, 1000, 100, iorder, 0, istor
  rireturn

PLAY:
  out aout
endin

instr 99
  if giorder != 8 then
    prints "resonx order did not change\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02
i 99 .03 .001
</CsScore>
</CsoundSynthesizer>
