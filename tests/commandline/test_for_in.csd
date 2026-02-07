<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1
seed(0)

/* for-in loop
   for var in array do
   ...
   od
   running either at i or k (perf) time
*/

/* case 1
   loop var is not declared
   loop type follows array type
   (i-type in this case)
*/
instr 1
for j in [1,2,3] do
 print j
od
endin

/* case 2
   loop var is declared
   loop type follows var type
   if a conversion is possible (e.g. k and i)
   regardless of array type
*/
instr 2
j:k init 0
for j in [1,2,3] do
 printk2 j
od
turnoff
endin

/* Loops of other array types are
   also possible 
*/
instr 3
for j in ["hello\n","world\n","!!!\n"] do
 prints j
od
endin

instr 4
  for r in [random:k(0,1),random:k(1,2)] do
     printk(0,r)
  od
endin

instr 5
  el:k init 0
  for a in [poscil:a(1,400),poscil:a(1,400,-1,1/4)] do
    printf("samples for element %d:\n",el+1,el)
    kndx = 0
    while (kndx < ksmps) do
      printk2(a[kndx])
      kndx += 1
    od
    el += 1
  od
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 0
i2 0.001 0.001
i3 0.002 1
i4 0.003 .001
i5 0.004 .001
</CsScore>
</CsoundSynthesizer>
