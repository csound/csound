<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

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


</CsInstruments>
<CsScore>
i1 0 0
i2 0 0.001
i3 0 0
</CsScore>
</CsoundSynthesizer>


