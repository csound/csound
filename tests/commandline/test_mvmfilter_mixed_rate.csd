<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  kcount init 0
  ain mpulse 1, 0
  kfreq = kcount < 6 ? 1000 : 1500
  ktau = kcount < 4 ? p4 : p5
  afreq = kfreq
  atau = ktau
  akk mvmfilter ain, kfreq, ktau
  aak mvmfilter ain, afreq, ktau
  aka mvmfilter ain, kfreq, atau
  aaa mvmfilter ain, afreq, atau
  kakerror max_k aak - akk, 1, 1
  kkaerror max_k aka - akk, 1, 1
  kaaerror max_k aaa - akk, 1, 1
  if !(kakerror < .00001 && kkaerror < .00001 && kaaerror < .00001) then
    printks "mvmfilter rate mismatch: ak=%g ka=%g aa=%g\n", 0, kakerror, kkaerror, kaaerror
    exitnowk(-1)
  endif

  ; Check the reference against the impulse response at sample 8.
  ksample downsamp akk
  if kcount == 1 then
    kexpected = 0
    if p4 > 0 then
      kexpected = exp(-ksmps / (sr * p4)) * cos(2 * $M_PI * 1000 * ksmps / sr)
    endif
    if !(abs(ksample - kexpected) < .00001) then
      printks "mvmfilter impulse mismatch: expected=%g actual=%g\n", 0, kexpected, ksample
      exitnowk(-1)
    endif
  endif
  if kcount == 10 then
    gkchecks += 1
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 5 then
    prints "not all mvmfilter checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Constant and changing decay, including the non-positive decay path.
i 1 0 .02 .01 .01
i 1 0 .02 0 0
i 1 0 .02 -.01 -.01
i 1 0 .02 .01 .02
i 1 0 .02 .01 0
i 99 .03 .001
</CsScore>
</CsoundSynthesizer>
