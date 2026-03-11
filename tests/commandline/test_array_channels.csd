<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>

instr 1
 S1[] fillarray "one", "two", "three"
 chnset S1, "string"
 i1[] fillarray 1, 2, 3
 chnset i1, "init" 
endin

instr 2

S1[] chnget "string"
if strcmp("one", S1[0]) != 0 then
  print "first string not matched\n"
  exitnow(-1)
elseif strcmp("two", S1[1]) != 0 then
  print "second string not matched\n"
  exitnow(-1)
elseif strcmp("three", S1[2]) != 0 then
  print "third string not matched\n"
  exitnow(-1)
else
  print "all strings in channel correctly matched\n"
endif


i1[] chnget "init"
if i1[0] != 1 then
  print "first number not matched\n"
  exitnow(-1)
elseif i1[1] != 2 then
  print "second number not matched\n"
  exitnow(-1)
elseif i1[2] != 3 then
  print "third number not matched\n"
  exitnow(-1)
else
  print "all numbers in init channel correctly matched\n"
endif

k1[] chnget "init"
if k1[0] != 1 then
  printks "first k number not matched\n", 1
  schedulek 3,0,0
elseif k1[1] != 2 then
  printks "second k  number not matched\n", 1
  schedulek 3,0,0
elseif k1[2] != 3 then
  printks "third k number not matched\n", 1
  schedulek 3,0,0
else
  printks "all k numbers in init channel correctly matched\n", 1
endif
endin

instr 3
exitnow(-1)
endin
</CsInstruments>

<CsScore>
i1 0 1
i2 0 1
</CsScore>
</CsoundSynthesizer>

