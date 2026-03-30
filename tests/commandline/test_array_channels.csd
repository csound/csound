<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
nchnls=2

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
a1 oscili 0dbfs, A4
asig[] = [a1, a1]
chnset asig, "audio"
aout[] chnget "audio"
   out aout
endin

instr 4 // other array types should work
arr:Complex[] init 2
arr[0] = 1,2
chnset arr, "test"
arr1:Complex[] chnget "test"
if real(arr1[0]) == real(arr[0]) then
  printk2 real(arr1[0])
else
  exitnowk(-1)
endif
endin


</CsInstruments>

<CsScore>
i1 0 1
i2 0 1
i3 0 1
i4 0 1
</CsScore>
</CsoundSynthesizer>

