<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
gkattack init 0
gkboundary init 0
gkenvelope init 0
gkreverse init 0
gkunused init 0

instr 1
  aout granule 1, 1, 1, 1, 0, 1, 1, 0, 0, 0.1, 0, 0, 0.002, 0, 0, 50, 0.5, 1, 1, 1, 1, 0
  ksample downsamp aout
  if gkattack == 0 then
    if !(abs(ksample - 1) <= 0.001) then
      printks "grain4 zero attack mismatch: expected %.3f, got %.3f\n", 0, 1, ksample
      exitnowk(-1)
    endif
  endif
  gkattack += 1
  if gkattack >= 1 then
    turnoff
  endif
endin

instr 2
  aout granule 1, 1, 1, 1, 0, 2, 1, 0, 0, 0.00051, 0, 0, 0.00201, 0, 0, 0, 0.5, 1.25, 1, 1, 1, 0
  ksample downsamp aout
  if gkboundary == 3 then
    if !(abs(ksample - 0.75) <= 0.001) then
      printks "grain4 boundary mismatch: expected %.3f, got %.3f\n", 0, 0.75, ksample
      exitnowk(-1)
    endif
  endif
  gkboundary += 1
  if gkboundary >= 4 then
    turnoff
  endif
endin

instr 3
  aout granule 1, 1, 1, 1, 0, 3, 1, 0, 0, 0.1, 0, 0, 0.00244140625, 0, 26, 51, 0.5, 1, 1, 1, 1, 4
  ksample downsamp aout
  if gkenvelope == 0 then
    if !(abs(ksample - 1) <= 0.001) then
      printks "grain4 envelope start mismatch: expected %.3f, got %.3f\n", 0, 1, ksample
      exitnowk(-1)
    endif
  elseif gkenvelope == 5 then
    if !(abs(ksample - 15) <= 0.001) then
      printks "grain4 envelope endpoint mismatch: expected %.3f, got %.3f\n", 0, 15, ksample
      exitnowk(-1)
    endif
  elseif gkenvelope == 12 then
    if !(abs(ksample - 12) <= 0.001) then
      printks "grain4 envelope mapping mismatch: expected %.3f, got %.3f\n", 0, 12, ksample
      exitnowk(-1)
    endif
  endif
  gkenvelope += 1
  if gkenvelope >= 13 then
    turnoff
  endif
endin

instr 4
  aout granule 1, 1, 1, -1, 0, 5, 1, 0, 0, 0.00051, 0, 0, 0.00201, 0, 0, 0, 0.5, 1.25, 1, 1, 1, 0
  ksample downsamp aout
  if gkreverse == 0 then
    if !(abs(ksample - 10) <= 0.001) then
      printks "grain4 reverse start mismatch: expected %.3f, got %.3f\n", 0, 10, ksample
      exitnowk(-1)
    endif
  elseif gkreverse == 7 then
    if !(abs(ksample - 32.5) <= 0.001) then
      printks "grain4 reverse wrap mismatch: expected %.3f, got %.3f\n", 0, 32.5, ksample
      exitnowk(-1)
    endif
  endif
  gkreverse += 1
  if gkreverse >= 8 then
    turnoff
  endif
endin

instr 5
  aout granule 1, 1, 1, 1, 0, 1, 1, 0, 0, 0.1, 0, 0, 0.002, 0, 0, 0, 0.5, 1, 0, 0, 0, 0
  ksample downsamp aout
  if !(abs(ksample - 1) <= 0.001) then
    printks "grain4 unused pitch mismatch: expected %.3f, got %.3f\n", 0, 1, ksample
    exitnowk(-1)
  endif
  gkunused += 1
  if gkunused >= 1 then
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 1024 7 1 1024 1
f 2 0 16 -2 0 1 2 3 99 99 99 99 99 99 99 99 99 99 99 99
f 3 0 1024 7 1 1024 1
f 4 0 16 -2 1 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
f 5 0 16 -2 10 20 30 40 99 99 99 99 99 99 99 99 99 99 99 99
i 1 0 0.002
i 2 0 0.001
i 3 0 0.002
i 4 0 0.001
i 5 0 0.001
</CsScore>
</CsoundSynthesizer>
