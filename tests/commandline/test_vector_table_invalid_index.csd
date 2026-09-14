<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef TEST_INDEX
#define TEST_INDEX #-1#
#endif
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1

giValues ftgen 1, 0, 4, -2, 10, 20, 30, 40

instr 1
  iindex strtod "$TEST_INDEX"
#ifdef TEST_ARATE
  aindex = iindex
  aout init 0
  vtablea aindex, giValues, 1, 0, aout
#else
#ifdef TEST_KRATE
  kindex = iindex
  kout init 0
  vtabk kindex, giValues, kout
#else
  iout init 0
  vtabi iindex, giValues, iout
#endif
#endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
