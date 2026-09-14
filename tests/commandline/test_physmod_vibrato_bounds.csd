<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkchecks init 0
giSine ftgen 1, 0, 16, 10, 1

instr 1, 2, 3, 4
  if p1 == 1 then
    aref wgclar .3, 440, -.3, .01, .01, 0, p5, .1, giSine
    atest wgclar .3, 440, -.3, .01, .01, 0, p4, .1, giSine
  elseif p1 == 2 then
    aref wgflute .3, 440, .32, .01, .01, 0, p5, .1, giSine
    atest wgflute .3, 440, .32, .01, .01, 0, p4, .1, giSine
  elseif p1 == 3 then
    aref wgbow .3, 220, 3, .2, p5, .1, giSine
    atest wgbow .3, 220, 3, .2, p4, .1, giSine
  else
    aref wgbrass .3, 440, .4, .01, p5, .1, giSine
    atest wgbrass .3, 440, .4, .01, p4, .1, giSine
  endif

  kerror max_k abs(atest - aref), 1, 1
  if kerror > .001 then
    printks "waveguide %d vibrato wrap mismatch: %g\n", 0, p1, kerror
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 11, 12, 13, 14
  kneg init -1
  kbad sqrt kneg
  if p1 == 11 then
    asig wgclar .3, 440, -.3, .01, .01, 0, kbad, .1, giSine
  elseif p1 == 12 then
    asig wgflute .3, 440, .32, .01, .01, 0, kbad, .1, giSine
  elseif p1 == 13 then
    asig wgbow .3, 220, 3, .2, kbad, .1, giSine
  else
    asig wgbrass .3, 440, .4, .01, kbad, .1, giSine
  endif
endin

instr 99
  if i(gkchecks) != 20 then
    prints "not all waveguide vibrato checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; These frequencies differ from 5 Hz by whole multiples of the sample rate.
i 1 0 .02 800005 5
i 2 .03 .02 800005 5
i 3 .06 .02 800005 5
i 4 .09 .02 800005 5
i 1 .12 .02 -799995 5
i 2 .15 .02 -799995 5
i 3 .18 .02 -799995 5
i 4 .21 .02 -799995 5
i 11 .24 .004
i 12 .25 .004
i 13 .26 .004
i 14 .27 .004
; Single-wrap paths in both directions.
i 1 .28 .02 8005 5
i 2 .31 .02 8005 5
i 3 .34 .02 8005 5
i 4 .37 .02 8005 5
i 1 .40 .02 -7995 5
i 2 .43 .02 -7995 5
i 3 .46 .02 -7995 5
i 4 .49 .02 -7995 5
; Adding the table length to a tiny negative phase can round to its endpoint.
i 1 .52 .02 -1e-20 0
i 2 .55 .02 -1e-20 0
i 3 .58 .02 -1e-20 0
i 4 .61 .02 -1e-20 0
i 99 .64 .001
</CsScore>
</CsoundSynthesizer>
