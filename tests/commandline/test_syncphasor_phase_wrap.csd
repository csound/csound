<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  asyncin = 0
  acps = p4
  akphase, aksync syncphasor p4, asyncin, p5
  aaphase, aasync syncphasor acps, asyncin, p5
  kkphase downsamp akphase
  kaphase downsamp aaphase
  kksync downsamp aksync
  kasync downsamp aasync
  kcount init 0
  if kcount == 0 then
    if !(abs(kkphase - p8) < .000001 && abs(kaphase - p8) < .000001) then
      printks "initial phase mismatch: cps=%g expected=%g k=%g a=%g\n", 0, p4, p8, kkphase, kaphase
      exitnowk(-1)
    endif
  elseif kcount == 1 then
    if !(abs(kkphase - p6) < .000001 && abs(kaphase - p6) < .000001 && kksync == p7 && kasync == p7) then
      printks "phase wrap mismatch: cps=%g expected=%g k=%g a=%g ksync=%g async=%g\n", 0, p4, p6, kkphase, kaphase, kksync, kasync
      exitnowk(-1)
    endif
    gkchecks += 1
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 6 then
    prints "not all syncphasor checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Frequency, initial phase, second phase, second sync, first phase.
i 1 0 .001 20000 0 .5 1 0
i 1 0 .001 -20000 0 .5 1 0
i 1 0 .001 8400 0 .05 1 0
i 1 0 .001 -8400 0 .95 1 0
i 1 0 .001 100 1.75 .7625 0 .75
i 1 0 .001 100 1e20 .0125 0 0
i 99 .002 .001
</CsScore>
</CsoundSynthesizer>
