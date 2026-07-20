<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
nchnls=2

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
  print "first string not matched"
  exitnow(-1)
elseif strcmp("two", S1[1]) != 0 then
  print "second string not matched"
  exitnow(-1)
elseif strcmp("three", S1[2]) != 0 then
  print "third string not matched"
  exitnow(-1)
else
  print "all strings in channel correctly matched"
endif


i1[] chnget "init"
if i1[0] != 1 then
  print "first number not matched"
  exitnow(-1)
elseif i1[1] != 2 then
  print "second number not matched"
  exitnow(-1)
elseif i1[2] != 3 then
  print "third number not matched"
  exitnow(-1)
else
  print "all numbers in init channel correctly matched"
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
turnoff
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
turnoff
endin

instr 5
arr:k[][] init 2,2
arr fillarray 1,2,3,4
chnset arr, "matrix"
mat:k[][] chnget "matrix"
i:k = 0
k:k = 1
while i < 2 do
 j:k = 0
 while j < 2 do 
  if mat[i][j] != k then
   printks "matrix[%d,%d] not matched\n", 1, i, j
   exitnowk(-1)
  endif
  j+=1
  k+=1
 od
 i+=1
od
printks "all matrix items correctly matched\n",1
turnoff
endin

struct duo var1:i, var2:i

instr 6
arr:duo[] init 2
arr[0].var1 = 1
arr[0].var2 = 2
arr[1].var1 = 3
arr[1].var2 = 4

chnset arr, "structs"
arr2:duo[] chnget "structs"
i:k = 0
while i < 2 do
 var1:k =  arr[i].var1
 var2:k =  arr2[i].var1
 if var1 != var2 then
    printks "struct var1 not matched for index %d\n",1,i
    exitnowk(-1)
 endif
 var1 =  arr[i].var2
 var2 =  arr2[i].var2
 if var1 != var2 then
    printks "struct var1 not matched for index %d\n",1,i
    exitnowk(-1)
 endif

i+=1
od
printks "all struct array items correctly matched\n",1
turnoff
endin


</CsInstruments>

<CsScore>
i1 0 1
i2 0 1
i3 0 1
i4 0 1
i5 0 1
i6 0 1

</CsScore>
</CsoundSynthesizer>
