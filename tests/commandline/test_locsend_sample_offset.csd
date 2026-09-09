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

instr 1, 2
  asig = .25
  if p1 == 1 then
    ad1, ad2 locsig asig, 45, 1, 1
    ar1, ar2 locsend
    ar3 = asig
    ar4 = asig
  else
    ad1, ad2, ad3, ad4 locsig asig, 45, 1, 1
    ar1, ar2, ar3, ar4 locsend
  endif
  kfirst init 1
  koffset offsetsmps
  if kfirst == 1 then
    if koffset != p4 then
      printks "wrong offset: expected=%g actual=%g\n", 0, p4, koffset
      exitnowk(-1)
    endif
    gkchecks += 1
    kfirst = 0
  endif
  ; At distance 1 and send amount 1, every send equals the input.
  kerr1 max_k ar1 - asig, 1, 1
  kerr2 max_k ar2 - asig, 1, 1
  kerr3 max_k ar3 - asig, 1, 1
  kerr4 max_k ar4 - asig, 1, 1
  if !(kerr1 < .000001 && kerr2 < .000001 && kerr3 < .000001 && kerr4 < .000001) then
    printks "locsend mismatch: %g %g %g %g\n", 0, kerr1, kerr2, kerr3, kerr4
    exitnowk(-1)
  endif
endin

instr 99
  if i(gkchecks) != 16 then
    prints "not all locsend offset checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Stereo and quad notes: aligned starts and offsets 1, 5, and 15.
i 1 0        .004 0
i 2 0        .004 0
i 1 .020125  .004 1
i 2 .020125  .004 1
i 1 .040625  .004 5
i 2 .040625  .004 5
i 1 .061875  .004 15
i 2 .061875  .004 15
; Three-sample notes also exercise partial end blocks.
i 1 .08      .000375 0
i 2 .08      .000375 0
i 1 .100125  .000375 1
i 2 .100125  .000375 1
i 1 .120625  .000375 5
i 2 .120625  .000375 5
i 1 .141875  .000375 15
i 2 .141875  .000375 15
i 99 .16 .001
</CsScore>
</CsoundSynthesizer>
