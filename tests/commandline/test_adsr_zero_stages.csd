<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef TEST_KSMPS
#define TEST_KSMPS #8#
#endif
sr = 8000
ksmps = $TEST_KSMPS
nchnls = 1
0dbfs = 1
gkchecks init 0

instr 1
  xtratim .012
  aa adsr p4, p5, p6, p7, p8
  am madsr p4, p5, p6, p7, p8, p7
  ka adsr p4, p5, p6, p7, p8
  km madsr p4, p5, p6, p7, p8, p7
  kaa downsamp aa
  kam downsamp am
  idelay = int(p8 * kr + .5)
  iattack = int(p4 * kr + .5)
  idecay = int(p5 * kr + .5)
  irelease = int(p7 * kr + .5)
  inote = int(p3 * kr + .5)
  kcount init 0
  kafter init 0
  krel release

  ; Piecewise linear reference, with no division for a zero-length stage.
  if kcount < idelay then
    kexpected = 0
  elseif kcount < idelay + iattack then
    kexpected = (kcount - idelay) / iattack
  elseif kcount < idelay + iattack + idecay then
    kexpected = 1 + (p6 - 1) * (kcount - idelay - iattack) / idecay
  else
    kexpected = p6
  endif
  kmexpected = kexpected
  if kcount >= inote then
    kexpected = 0
  elseif kcount >= inote - irelease then
    kexpected = p6 * (inote - kcount) / irelease
  endif
  if krel == 1 then
    if kafter >= irelease then
      kmexpected = 0
    else
      kmexpected = p6 * (1 - kafter / irelease)
    endif
    if kafter == int(.008 * kr + .5) then
      gkchecks += 1
    endif
    kafter += 1
  endif
  if !(abs(kaa-kexpected) < .00001 && abs(ka-kexpected) < .00001 && abs(kam-kmexpected) < .00001 && abs(km-kmexpected) < .00001) then
    printks "ADSR curve mismatch at block %g, params %g %g %g %g %g: adsr %g %g expected %g; madsr %g %g expected %g\n", 0, kcount, p4, p5, p6, p7, p8, kaa, ka, kexpected, kam, km, kmexpected
    exitnowk(-1)
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 32 then
    prints "not all ADSR curves reached release\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; All zero/nonzero combinations of attack, decay, sustain, release, delay.
i 1 0 .02 0 0 0 0 0
i 1 0 .02 0 0 0 0 0.002
i 1 0 .02 0 0 0 0.002 0
i 1 0 .02 0 0 0 0.002 0.002
i 1 0 .02 0 0 0.5 0 0
i 1 0 .02 0 0 0.5 0 0.002
i 1 0 .02 0 0 0.5 0.002 0
i 1 0 .02 0 0 0.5 0.002 0.002
i 1 0 .02 0 0.002 0 0 0
i 1 0 .02 0 0.002 0 0 0.002
i 1 0 .02 0 0.002 0 0.002 0
i 1 0 .02 0 0.002 0 0.002 0.002
i 1 0 .02 0 0.002 0.5 0 0
i 1 0 .02 0 0.002 0.5 0 0.002
i 1 0 .02 0 0.002 0.5 0.002 0
i 1 0 .02 0 0.002 0.5 0.002 0.002
i 1 0 .02 0.002 0 0 0 0
i 1 0 .02 0.002 0 0 0 0.002
i 1 0 .02 0.002 0 0 0.002 0
i 1 0 .02 0.002 0 0 0.002 0.002
i 1 0 .02 0.002 0 0.5 0 0
i 1 0 .02 0.002 0 0.5 0 0.002
i 1 0 .02 0.002 0 0.5 0.002 0
i 1 0 .02 0.002 0 0.5 0.002 0.002
i 1 0 .02 0.002 0.002 0 0 0
i 1 0 .02 0.002 0.002 0 0 0.002
i 1 0 .02 0.002 0.002 0 0.002 0
i 1 0 .02 0.002 0.002 0 0.002 0.002
i 1 0 .02 0.002 0.002 0.5 0 0
i 1 0 .02 0.002 0.002 0.5 0 0.002
i 1 0 .02 0.002 0.002 0.5 0.002 0
i 1 0 .02 0.002 0.002 0.5 0.002 0.002
i 99 .04 .001
</CsScore>
</CsoundSynthesizer>
