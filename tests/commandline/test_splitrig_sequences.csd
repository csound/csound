<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giData ftgen 1, 0, -8, -2, 2, 11, 12, 21, 22, 1, 31, 32
giGuard ftgen 2, 0, -4, -2, 2, 41, 42, 51, 52
gkchecks init 0

instr 1
  kstep init 0
  kfirst init 0
  ksecond init 0
  kexpected[] fillarray 11, 21, 11, 11, 31, 0, 11, 11, 21
  kindex = (kstep == 4 ? 1.5 : 0)
  ktrig = (kstep == 5 ? 0 : 1)
  kcount = (kstep >= 3 && kstep < 7 ? 1 : 2)
  tablew kcount, 0, giData
  splitrig ktrig, kindex, 2, giData, kfirst, ksecond
  ksecondExpected = (ktrig == 0 ? 0 : kexpected[kstep] + 1)
  if kfirst != kexpected[kstep] || ksecond != ksecondExpected then
    printks "splitrig step %d: got %g, %g\n", 0, kstep, kfirst, ksecond
    exitnowk(-1)
  endif
  kstep += 1
  if kstep == 9 then
    gkchecks += 1
    turnoff
  endif
endin
; The last output of the second tick occupies the guard point.
instr 2
  kstep init 0
  kfirst init 0
  ksecond init 0
  splitrig 1, -.5, 2.75, giGuard, kfirst, ksecond
  kexpected = (kstep % 2 == 0 ? 41 : 51)
  if kfirst != kexpected || ksecond != kexpected + 1 then
    printks "splitrig did not read the final tick at the guard point\n", 0
    exitnowk(-1)
  endif
  kstep += 1
  if kstep == 4 then
    gkchecks += 1
    turnoff
  endif
endin

; A single sequence need not reserve space for all imaxtics ticks.
instr 3
  kfirst init 0
  ksecond init 0
  splitrig 1, 0, 2147483520, giGuard, kfirst, ksecond
  kstep init 0
  kexpected = (kstep % 2 == 0 ? 41 : 51)
  if kfirst != kexpected || ksecond != kexpected + 1 then
    printks "splitrig misread a sequence with a large stride\n", 0
    exitnowk(-1)
  endif
  kstep += 1
  if kstep == 4 then
    gkchecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkchecks) != 3 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
