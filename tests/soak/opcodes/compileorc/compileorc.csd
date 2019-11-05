<CsoundSynthesizer>
<CsInstruments>
sr = 44100
nchnls = 1
ksmps = 32
0dbfs = 1

instr 1
  ires compileorc "my.orc"
  print ires ; 0 as compiled successfully
  event_i "i", 2, 0, 3, .2, 465 ;send event
  ires compileorc "does_not_exist.orc"
  print ires ; -1 as could not compile
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
