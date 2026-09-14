<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giTable ftgen 1, 0, -4, -2, 1, 2, 3, 4, 7
gkchecks init 0

instr 1
  ksum tabsum 1, p4, p5
  if !(abs(ksum - p6) <= 0.000001) then
    printks "tabsum range %g to %g: expected %g, got %g\n", 0, p4, p5, p6, ksum
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin
instr 2
  kcycle init 0
  kmax = 1 + kcycle % 4
  ksum tabsum 1, 1, kmax
  kexpected[] fillarray 2, 5, 9, 16
  if !(abs(ksum - kexpected[kcycle % 4]) <= .000001) then
    printks "tabsum did not follow the changing range\n", 0
    exitnowk(-1)
  endif
  if kcycle == 7 then
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 99
  if i(gkchecks) != 12 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0    0.01 0 0 10
i 1 0.02 0.01 0 3 10
i 1 0.04 0.01 3 1 9
i 1 0.06 0.01 1 2 5
; Explicit endpoints may include the guard point; the default excludes it.
i 1 0 .01 0 4 17
i 1 0 .01 4 4 7
i 1 0 .01 4 2 14
; Round fractional endpoints as before, including either table boundary.
i 1 0 .01 0 3.6 17
i 1 0 .01 0 4.25 17
i 1 0 .01 -.25 3 10
i 1 0 .01 -.25 .25 10
i 2 0 .01
i 99 .08 .001
</CsScore>
</CsoundSynthesizer>
