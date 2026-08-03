<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giTrajectory ftgen 1, 0, 0, -28, "gen28_terminal_point.txt"

instr 1
  iLastY = table:i(3, giTrajectory)
  if abs(iLastY - 0.5) > 0.000001 then
    prints "GEN28 terminal y mismatch: %f\n", iLastY
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
