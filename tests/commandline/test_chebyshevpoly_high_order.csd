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

#define Z8 #0, 0, 0, 0, 0, 0, 0, 0#

instr 1
  kcycle init 0
  ain = p4
  kscale = kcycle < 2 ? 1 : -.25
  ; T64 alone exposes the loss from converting to power coefficients.
  aresult chebyshevpoly2 ain, $Z8, $Z8, $Z8, $Z8, $Z8, $Z8, $Z8, $Z8, kscale
  kcoeff[] fillarray $Z8, $Z8, $Z8, $Z8, $Z8, $Z8, $Z8, $Z8, kscale
  aarray chebyshevpoly2 ain, kcoeff
  aexpected = p5 * kscale
  kerror max_k abs(aresult - aexpected) + abs(aarray - aexpected), 1, 1
  if !(kerror < .0001) then
    printks "T64 mismatch: input=%g expected=%g error=%g\n", 0, p4, p5 * kscale, kerror
    exitnowk(-1)
  endif
  if kcycle == 5 then
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 2
  kcycle init 0
  ain = p4
  kscale = kcycle < 2 ? 1 : -.25
  aresult chebyshevpoly2 ain, .3 * kscale, -.2 * kscale,
                               .7 * kscale, 0, -.5 * kscale,
                               .1 * kscale, 0, 0, .4 * kscale
  alegacy chebyshevpoly ain, .3 * kscale, -.2 * kscale,
                             .7 * kscale, 0, -.5 * kscale,
                             .1 * kscale, 0, 0, .4 * kscale
  aexpected = p5 * kscale
  kerror max_k abs(aresult - aexpected) + abs(alegacy - aexpected), 1, 1
  if !(kerror < .0001) then
    printks "mixed Chebyshev mismatch: input=%g expected=%g error=%g\n", 0, p4, p5 * kscale, kerror
    exitnowk(-1)
  endif
  if kcycle == 5 then
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 3
  ain = p4
  aresult chebyshevpoly2 ain, p5
  icoeff[] fillarray p5
  aarray chebyshevpoly2 ain, icoeff
  alinear chebyshevpoly2 ain, p5, .5
  ilinear[] fillarray p5, .5
  alinearArray chebyshevpoly2 ain, ilinear
  aexpectedLinear = p5 + .5 * ain
  kerror max_k abs(aresult - p5) + abs(aarray - p5) +
               abs(alinear - aexpectedLinear) +
               abs(alinearArray - aexpectedLinear), 1, 1
  if !(kerror < .000001) then
    printks "constant Chebyshev mismatch: expected=%g error=%g\n", 0, p5, kerror
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 99
  if i(gkchecks) != 6 then
    prints "not all chebyshevpoly checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 .9 -.8301709349
i 1 0 .01 1 1
i 1 0 .01 -1 1
i 2 0 .01 .75 .98359375
i 2 0 .01 -.4 -.542259968
i 3 0 .01 .9 .25
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
