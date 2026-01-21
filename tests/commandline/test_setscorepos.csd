<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
gi1 init 0
instr 1
  prints "Top...\n"
  setscorepos(gi1)
endin
instr 2
  gi1 = 3
  prints "Rewinding...\n"
  rewindscore()
endin
</CsInstruments>
<CsScore>
i 1 0 1
i 2 1 0 
</CsScore>
</CsoundSynthesizer>


