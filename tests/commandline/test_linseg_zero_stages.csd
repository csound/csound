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
  aa linseg 0, .002, 1, 0, .25, .002, .75, 0, .5, .002, 0
  kk linseg 0, .002, 1, 0, .25, .002, .75, 0, .5, .002, 0
  ar linsegr 0, .002, 1, 0, .25, .002, .75, 0, .5, .002, 0
  krvalue linsegr 0, .002, 1, 0, .25, .002, .75, 0, .5, .002, 0
  ap linseg 0, .002, 1, .002, .75, .002, 0
  kp linseg 0, .002, 1, .002, .75, .002, 0
  ka downsamp aa
  kar downsamp ar
  kap downsamp ap
  ilength = int(.002 * kr + .5)
  kcount init 0
  kafter init 0
  krel release
  if kcount < ilength then
    kexpected = kcount / ilength
    kpositive = kexpected
  elseif kcount < 2 * ilength then
    kexpected = .25 + .5 * (kcount - ilength) / ilength
    kpositive = 1 - .25 * (kcount - ilength) / ilength
  elseif kcount < 3 * ilength then
    kexpected = .5 * (3 - kcount / ilength)
    kpositive = .75 * (3 - kcount / ilength)
  else
    kexpected = 0
    kpositive = 0
  endif
  krexpected = kcount < 2 * ilength ? kexpected : .5
  if krel == 1 then
    krexpected = .5 * max(1 - kafter / ilength, 0)
    if kafter == ilength then
      gkchecks += 1
    endif
    kafter += 1
  endif
  if !(abs(ka-kexpected) < .00001 && abs(kk-kexpected) < .00001 && abs(kar-krexpected) < .00001 && abs(krvalue-krexpected) < .00001 && abs(kap-kpositive) < .00001 && abs(kp-kpositive) < .00001) then
    printks "linseg timing mismatch at block %g: linear %g %g expected %g; release %g %g expected %g; positive %g %g expected %g\n", 0, kcount, ka, kk, kexpected, kar, krvalue, krexpected, kap, kp, kpositive
    exitnowk(-1)
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 1 then
    prints "linsegr did not complete release\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02
i 99 .04 .001
</CsScore>
</CsoundSynthesizer>
