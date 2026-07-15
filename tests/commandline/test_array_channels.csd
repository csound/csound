<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

clearDimensions@global:i[] fillarray 2
chnarray "clear", 3, "k", clearDimensions

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
  exitnowk(-1)
elseif k1[1] != 2 then
  printks "second k  number not matched\n", 1
  exitnowk(-1)
elseif k1[2] != 3 then
  printks "third k number not matched\n", 1
  exitnowk(-1)
else
  printks "all k numbers in init channel correctly matched\n", 1
endif
endin

instr 3
values:k[] fillarray 3, 4
chnset values, "clear"
chncleararray "clear"
cleared:k[] init 2
cleared chnget "clear"

if cleared[0] != 0 || cleared[1] != 0 then
  printks "array channel was not cleared: %g, %g\n", 0, \
    cleared[0], cleared[1]
  exitnowk(-1)
endif
turnoff
endin
</CsInstruments>

<CsScore>
i1 0 1
i2 0 1
i3 0 1
</CsScore>
</CsoundSynthesizer>
