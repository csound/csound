<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
gi1 init 2
instr 1
  prints "Top...\n"
  setscorepos(gi1)
endin

instr 2
  gi1 = 4
  prints "Rewinding...\n"
  rewindscore()
endin

instr 3
  // should not reach this if setscorepos() and
  // rewindscore() worked
  exitnow(-1)
endin

</CsInstruments>
<CsScore>
i 1 0 1
i 3 1 0
i 2 2 0
i 3 3 0
</CsScore>
</CsoundSynthesizer>


