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

instr 1, 2
  seed 1
  acps = sr
  if p1 == 1 then
    arandom randomi 0, 1, acps, 2, .25
  else
    arandom randomh 0, 1, acps, 2, .25
  endif

  ksample downsamp arandom
  kcount init 0
  if kcount == 1 then
    if abs(ksample - .25) < .000001 then
      printks "random opcode used an inactive rate sample: type=%g\n", 0, p1
      exitnowk(-1)
    endif
    gkchecks += 1
  endif
  kcount += 1
endin

instr 3, 4, 5, 6
  iNaN strtod "nan"
  iHuge strtod "1e100"
  if p4 == 1 then
    iRate = iNaN
  elseif p4 == 2 then
    iRate = -1
  elseif p4 == 3 then
    iRate = iHuge
  elseif p4 == 4 then
    iRate = 0
  else
    iRate = (p1 <= 4 ? sr : kr) * 2.5
  endif

  if p1 == 3 then
    acps = iRate
    arandom randomi 0, 1, acps, 2, .25
    kdeviation max_k arandom - .5, 1, 1
    kvalue downsamp arandom
  elseif p1 == 4 then
    acps = iRate
    arandom randomh 0, 1, acps, 2, .25
    kdeviation max_k arandom - .5, 1, 1
    kvalue downsamp arandom
  elseif p1 == 5 then
    kcps = iRate
    krandom randomi 0, 1, kcps, 2, .25
    kdeviation = abs(krandom - .5)
    kvalue = krandom
  else
    kcps = iRate
    krandom randomh 0, 1, kcps, 2, .25
    kdeviation = abs(krandom - .5)
    kvalue = krandom
  endif

  kfirst init 1
  if qnan(kvalue) != 0 || qnan(kdeviation) != 0 || \
     kdeviation > .500001 then
    printks "random opcode produced an invalid value: type=%g rate=%g\n", \
             0, p1, p4
    exitnowk(-1)
  endif
  if (p4 < 3 || p4 == 4) && abs(kvalue - .25) > .000001 then
    printks "random opcode advanced for an invalid rate: type=%g rate=%g\n", \
             0, p1, p4
    exitnowk(-1)
  endif
  if kfirst == 0 && (p4 == 3 || p4 == 5) && abs(kvalue - .25) < .000001 then
    printks "random opcode did not advance at a high rate: type=%g\n", 0, p1
    exitnowk(-1)
  endif
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

; Whole extra cycles must not change the fractional phase remainder.
instr 7, 8
  seed 1
  ifirst random 0, 1
  isecond random 0, 1
  seed 1
  if p1 == 7 then
    kresult randomi 0, 1, p4 * kr, 2, .25
    iexpected = (ifirst + isecond) / 2
  else
    kresult randomh 0, 1, p4 * kr, 2, .25
    iexpected = ifirst
  endif
  kcount init 0
  if kcount == 1 && abs(kresult - iexpected) > .000001 then
    printks "random opcode lost the phase remainder: type=%g value=%g\n", 0, p1, kresult
    exitnowk(-1)
  endif
  if kcount == 2 then
    if abs(kresult - isecond) > .000001 then
      printks "random opcode advanced the wrong number of times: type=%g\n", 0, p1
      exitnowk(-1)
    endif
    gkchecks += 1
  endif
  kcount += 1
endin

instr 99
  if i(gkchecks) != 26 then
    prints "not all random rate checks ran\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 .001875 .01
i 2 .021875 .01
i 3 .05 .004 1
i 3 .06 .004 2
i 3 .07 .004 3
i 4 .08 .004 1
i 4 .09 .004 2
i 4 .10 .004 3
i 5 .11 .004 1
i 5 .12 .004 2
i 5 .13 .004 3
i 6 .14 .004 1
i 6 .15 .004 2
i 6 .16 .004 3
i 3 .18 .004 4
i 4 .19 .004 4
i 5 .20 .004 4
i 6 .21 .004 4
i 3 .22 .004 5
i 4 .23 .004 5
i 5 .24 .004 5
i 6 .25 .004 5
i 7 .27 .008 1.5
i 8 .29 .008 1.5
i 7 .31 .008 2.5
i 8 .33 .008 2.5
i 99 .35 .001
</CsScore>
</CsoundSynthesizer>
