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
giLinear ftgen 0, 0, 32, -2, 0, 0, 0, 0.25, 0.5, 0.75, 1, 0.75, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5
giExponential ftgen 0, 0, 32, -2, 0, 0, 0.001, 0.005623413251903491, 0.031622776601683791, 0.17782794100389229, 1, 0.70710678118654757, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5

instr 1, 2, 3, 4
  if p1 == 1 then
    aenv adsr .0005, .00025, .5, .004, .00025
  elseif p1 == 2 then
    aenv madsr .0005, .00025, .5, .004, .00025
  elseif p1 == 3 then
    aenv xadsr .0005, .00025, .5, .004, .00025
  else
    aenv mxadsr .0005, .00025, .5, .004, .00025
  endif
  aphase phasor sr / 32
  itable = p1 < 3 ? giLinear : giExponential
  aref table aphase * 32, itable
  kerror max_k aenv - aref, 1, 1
  koffset offsetsmps
  kfirst init 1
  if kfirst == 1 then
    if !(koffset == p4 && kerror < .00001) then
      printks "ADSR start mismatch: type=%g offset=%g expected offset=%g error=%g\n", 0, p1, koffset, p4, kerror
      exitnowk(-1)
    endif
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 99
  if i(gkchecks) != 16 then
    prints "not all ADSR offset checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 0
i 2 0 .008 0
i 3 0 .008 0
i 4 0 .008 0
i 1 0.020125 .008 1
i 2 0.020125 .008 1
i 3 0.020125 .008 1
i 4 0.020125 .008 1
i 1 0.040625 .008 5
i 2 0.040625 .008 5
i 3 0.040625 .008 5
i 4 0.040625 .008 5
i 1 0.061875 .008 15
i 2 0.061875 .008 15
i 3 0.061875 .008 15
i 4 0.061875 .008 15
i 99 .09 .001
</CsScore>
</CsoundSynthesizer>
