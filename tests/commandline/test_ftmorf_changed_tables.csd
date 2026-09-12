<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
giSource ftgen 1, 0, 8, -2, 1
giShort ftgen 2, 0, 4, -2, 1
instr 1
  iList ftgen 0, 0, 2, -2, 1, 1
  iResult ftgen 0, 0, 8, -2, 0
  ; Change one entry after ftmorf has checked the original list at init.
  kTable = p4
  tablew kTable, p5, iList
  ftmorf .5, iList, iResult
endin
</CsInstruments>
<CsScore>
; Two performance errors: a short table in either source position.
i 1 0 .01 2 0
i 1 0 .01 2 1
e
</CsScore>
</CsoundSynthesizer>
