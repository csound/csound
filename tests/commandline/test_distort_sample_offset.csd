<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 64000
ksmps = 8
nchnls = 1
0dbfs = 1

gishape ftgen 0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1
gkchecks init 0

instr 1
  kcycle init 0
  asignal init 0
  aresult distort asignal, 1, gishape
  kresult downsamp aresult
  if kcycle == 1 then
    iexpected = 1 / 32768
    if abs(kresult - iexpected) > .000000001 then
      printks "FAIL expected=%g actual=%g\n", 0, iexpected, kresult
      exitnowk(-1)
    endif
    gkchecks += 1
  endif
  kcycle += 1
endin

instr 99
  if i(gkchecks) != 1 then
    prints "distort onset check did not run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 .000046875 .001
i 99 .002 .001
</CsScore>
</CsoundSynthesizer>
