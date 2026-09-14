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
giSine ftgen 1, 0, 1024, 10, 1

instr 1, 2, 3, 4, 5, 6
  afrq1 = 1
  afrq2 = 1.5
  andx1 = .7
  andx2 = .9
  kfrq1 init 1
  kfrq2 init 1.5
  kndx1 init .7
  kndx2 init .9

  if p1 == 1 then
    aa1, aa2 crossfm afrq1, afrq2, andx1, andx2, 400, giSine, giSine
    ak1, ak2 crossfm kfrq1, kfrq2, kndx1, kndx2, 400, giSine, giSine
    am1, am2 crossfm afrq1, kfrq2, andx1, kndx2, 400, giSine, giSine
    an1, an2 crossfm kfrq1, afrq2, kndx1, andx2, 400, giSine, giSine
  elseif p1 == 2 then
    aa1, aa2 crossfmi afrq1, afrq2, andx1, andx2, 400, giSine, giSine
    ak1, ak2 crossfmi kfrq1, kfrq2, kndx1, kndx2, 400, giSine, giSine
    am1, am2 crossfmi afrq1, kfrq2, andx1, kndx2, 400, giSine, giSine
    an1, an2 crossfmi kfrq1, afrq2, kndx1, andx2, 400, giSine, giSine
  elseif p1 == 3 then
    aa1, aa2 crosspm afrq1, afrq2, andx1, andx2, 400, giSine, giSine
    ak1, ak2 crosspm kfrq1, kfrq2, kndx1, kndx2, 400, giSine, giSine
    am1, am2 crosspm afrq1, kfrq2, andx1, kndx2, 400, giSine, giSine
    an1, an2 crosspm kfrq1, afrq2, kndx1, andx2, 400, giSine, giSine
  elseif p1 == 4 then
    aa1, aa2 crosspmi afrq1, afrq2, andx1, andx2, 400, giSine, giSine
    ak1, ak2 crosspmi kfrq1, kfrq2, kndx1, kndx2, 400, giSine, giSine
    am1, am2 crosspmi afrq1, kfrq2, andx1, kndx2, 400, giSine, giSine
    an1, an2 crosspmi kfrq1, afrq2, kndx1, andx2, 400, giSine, giSine
  elseif p1 == 5 then
    aa1, aa2 crossfmpm afrq1, afrq2, andx1, andx2, 400, giSine, giSine
    ak1, ak2 crossfmpm kfrq1, kfrq2, kndx1, kndx2, 400, giSine, giSine
    am1, am2 crossfmpm afrq1, kfrq2, andx1, kndx2, 400, giSine, giSine
    an1, an2 crossfmpm kfrq1, afrq2, kndx1, andx2, 400, giSine, giSine
  else
    aa1, aa2 crossfmpmi afrq1, afrq2, andx1, andx2, 400, giSine, giSine
    ak1, ak2 crossfmpmi kfrq1, kfrq2, kndx1, kndx2, 400, giSine, giSine
    am1, am2 crossfmpmi afrq1, kfrq2, andx1, kndx2, 400, giSine, giSine
    an1, an2 crossfmpmi kfrq1, afrq2, kndx1, andx2, 400, giSine, giSine
  endif

  kerr1 max_k abs(aa1 - ak1), 1, 1
  kerr2 max_k abs(aa2 - ak2), 1, 1
  kerr3 max_k abs(am1 - ak1) + abs(am2 - ak2), 1, 1
  kerr4 max_k abs(an1 - ak1) + abs(an2 - ak2), 1, 1
  if kerr1 > .000001 || kerr2 > .000001 || kerr3 > .000001 || kerr4 > .000001 then
    printks "crossfm%d rate mismatch: errors=%g,%g,%g,%g\n", \
             0, p1, kerr1, kerr2, kerr3, kerr4
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    koffset offsetsmps
    if koffset != 5 then
      printks "crossfm%d offset mismatch: offset=%d errors=%g,%g\n", \
               0, p1, koffset, kerr1, kerr2
      exitnowk(-1)
    endif
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 11, 12, 13, 14, 15, 16
  aneg init -1
  if p4 == 0 then
    abad sqrt aneg
  else
    abad = p4
  endif
  if p1 == 11 then
    a1, a2 crossfm abad, abad, abad, abad, 400, giSine, giSine
  elseif p1 == 12 then
    a1, a2 crossfmi abad, abad, abad, abad, 400, giSine, giSine
  elseif p1 == 13 then
    a1, a2 crosspm abad, abad, abad, abad, 400, giSine, giSine
  elseif p1 == 14 then
    a1, a2 crosspmi abad, abad, abad, abad, 400, giSine, giSine
  elseif p1 == 15 then
    a1, a2 crossfmpm abad, abad, abad, abad, 400, giSine, giSine
  else
    a1, a2 crossfmpmi abad, abad, abad, abad, 400, giSine, giSine
  endif
endin

instr 99
  if i(gkchecks) != 6 then
    prints "not all crossfm checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 .000625 .01
i 2 .020625 .01
i 3 .040625 .01
i 4 .060625 .01
i 5 .080625 .01
i 6 .100625 .01
i 11 .12 .004
i 12 .13 .004
i 13 .14 .004
i 14 .15 .004
i 15 .16 .004
i 16 .17 .004
; Tiny negative increments round to a wrapped phase of 1 without correction.
i 11 .18 .004 -1e-20
i 12 .19 .004 -1e-20
i 13 .20 .004 -1e-20
i 14 .21 .004 -1e-20
i 15 .22 .004 -1e-20
i 16 .23 .004 -1e-20
i 99 .24 .001
</CsScore>
</CsoundSynthesizer>
