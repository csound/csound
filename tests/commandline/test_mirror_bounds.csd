<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  iscale = 2 ^ p8
  ix = p4 * iscale
  ilow = p5 * iscale
  ihigh = p6 * iscale
  ivalue mirror ix, ilow, ihigh
  if !(abs(ivalue / iscale - p7) < .000001) then
    prints "mirror i-rate mismatch: input=%g low=%g high=%g expected=%g got=%g\n", p4, p5, p6, p7, ivalue / iscale
    exitnow(-1)
  endif
  kx = ix
  ax = ix
  kvalue mirror kx, ilow, ihigh
  avalue mirror ax, ilow, ihigh
  aexpected = p7
  kerror max_k abs(avalue / iscale - aexpected), 1, 1
  if !(abs(kvalue / iscale - p7) < .000001 && kerror < .000001) then
    printks "mirror mismatch: input=%g low=%g high=%g expected=%g k=%g a-error=%g\n", 0, p4, p5, p6, p7, kvalue / iscale, kerror
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 99
  if i(gkchecks) != 27 then
    prints "not all mirror checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Input, lower bound, upper bound, expected result, binary scale exponent.
i 1 0 .004 -3.25 0 1 .75 0
i 1 0 .004 -2 0 1 0 0
i 1 0 .004 -1 0 1 1 0
i 1 0 .004 -.5 0 1 .5 0
i 1 0 .004 0 0 1 0 0
i 1 0 .004 .25 0 1 .25 0
i 1 0 .004 1 0 1 1 0
i 1 0 .004 1.25 0 1 .75 0
i 1 0 .004 2 0 1 0 0
i 1 0 .004 3 0 1 1 0
i 1 0 .004 1e20 0 1 0 0
i 1 0 .004 -1e20 0 1 0 0
i 1 0 .004 1e20 1 2 2 0
i 1 0 .004 -1e20 1 2 2 0
i 1 0 .004 -3 -2 3 -1 0
i 1 0 .004 4 -2 3 2 0
i 1 0 .004 9 -2 3 -1 0
i 1 0 .004 0 3 3 3 126
i 1 0 .004 0 3 2 2.5 126
i 1 0 .004 3 -1 1 -1 126
i 1 0 .004 3 -3 2 1 126
i 1 0 .004 3 -3 -2 -3 126
i 1 0 .004 2.5 -3 -2 -2.5 126
i 1 0 .004 7 0 3 1 -120
; Check notes that start and end within a control block.
i 1 .000125 .003375 4 -2 3 2 0
i 1 .000625 .003375 4 -2 3 2 0
i 1 .001875 .003375 4 -2 3 2 0
i 99 .01 .002
</CsScore>
</CsoundSynthesizer>
