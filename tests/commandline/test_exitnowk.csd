<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
if timeinstk() == 1 then
  if 0 != 0 then
    exitnowk(-1)
  endif
endif

if timeinstk() > 1 then
  exitnowk(0)
endif
endin

instr 2
prints "exitnowk failed to stop later events\n"
exitnow(-1)
endin

</CsInstruments>

<CsScore>
i1 0 1
i2 0.5 0.01
e 2
</CsScore>
</CsoundSynthesizer>
