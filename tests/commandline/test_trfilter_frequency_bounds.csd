<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

giSine ftgen 1, 0, 8192, 10, 1
giFilter ftgen 2, 0, -16, -7, 1, 16, 2

instr 1
  kchecks init 0
  kcycle timeinstk
  asig oscili .5, 1000, giSine
  ffreq, fphase pvsifd asig, 64, 16, 1
  ftracks partials ffreq, fphase, .001, 1, 3, 10
  fscaled trscale ftracks, p4
  ffiltered trfilter fscaled, p5, giFilter
  flow1, kfreq1, kamp1 trlowest ftracks, 1
  if p4 == 0 then
    flow2, kfreq2, kamp2 trlowest ffiltered, 1
  else
    fhigh2, kfreq2, kamp2 trhighest ffiltered, 1
  endif

  if kamp1 > 0 then
    kratio = kamp2 / kamp1
    kexpected = min(abs(kfreq1 * p4), sr / 2)
    kgain = 1 + kexpected / (sr / 2) * p5
    if kamp2 <= 0 || abs(kfreq2 - kexpected) > .001 || abs(kratio - kgain) > .000001 then
      printks "trfilter bounds failed: frequency=%f ratio=%f\n", 0, kfreq2, kratio
      exitnowk(-1)
    endif
    kchecks += 1
  endif
  if kcycle == 90 && kchecks == 0 then
    printks "trfilter produced no track to check\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Beyond Nyquist, at the endpoint, at DC, and inside the table.
i 1 0 .1 10 1
i 1 .1 .1 -10 1
i 1 .2 .1 4 1
i 1 .3 .1 -4 1
i 1 .4 .1 0 1
i 1 .5 .1 1 1
i 1 .6 .1 -1 1
i 1 .7 .1 .713 1
; Partial filtering and bypass keep their existing gain.
i 1 .8 .1 .713 .5
i 1 .9 .1 -10 0
</CsScore>
</CsoundSynthesizer>
