<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
#ifndef TEST_KSMPS
#define TEST_KSMPS #8#
#endif
ksmps = $TEST_KSMPS
nchnls = 1
0dbfs = 1
instr 1
  kcount init 0
  kphase init 0
  if kcount == int(.008 * kr + .5) then
    kphase = 1
    reinit ENVELOPES
  endif
ENVELOPES:
  iattack = i(kphase) == 0 ? .001 : 0
  ilevel = i(kphase) == 0 ? .75 : .25
  aa adsr iattack, .001, ilevel, .002
  am madsr iattack, .001, ilevel, .002
  ka adsr iattack, .001, ilevel, .002
  km madsr iattack, .001, ilevel, .002
  rireturn
  kaa downsamp aa
  kam downsamp am
  if kcount == int(.015 * kr + .5) then
    printks "after reinit: adsr a=%g k=%g; madsr a=%g k=%g (expected .25)\n", 0, kaa, ka, kam, km
    if !(abs(kaa-.25) < .000001 && abs(ka-.25) < .000001 && abs(kam-.25) < .000001 && abs(km-.25) < .000001) then
      exitnowk(-1)
    endif
  endif
  kcount += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .03
</CsScore>
</CsoundSynthesizer>
