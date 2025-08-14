<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1
seed(0)

instr 1
for j, i in [2,4,6] do
 print j, i
od
endin

instr 2
  arr:k[] init 5
  printarray(arr,1,"%.3f","array before fill")
  for el,indx in arr do
    arr[indx] = rnd(1)
  od
  printarray(arr,1,"%.3f","array after fill")
  turnoff
endin

instr 3
for S, i in ["hello ","world ","!!! "] do
 puts(S,1)
 print i
od
endin

instr 4
  for r,i in [random:k(0,1),random:k(1,2)] do
    printk2 i
    printk(0,r)
  od
endin

instr 5
  el1:a = poscil:a(1,441)
  el2:a = poscil:a(1,441,-1,1/4)
  myarr:a[] = [el1,el2]
  // print the audio vector for both elements
  for a,i in myarr do
    printk2(i)
    kndx = 0
    while (kndx < ksmps) do
      printk2(a[kndx],10)
      kndx += 1
    od
  od
  // now swap the elements and print again
  myarr = [el2,el1]
  for a,i in myarr do
    printk2(i)
    kndx = 0
    while (kndx < ksmps) do
      printk2(a[kndx],10)
      kndx += 1
    od
  od
  turnoff
endin


</CsInstruments>
<CsScore>
i1 0 0
i 2 .001 .01
i 3 .002 0
i 4 .003 .001
i 5 .004 .001
</CsScore>
</CsoundSynthesizer>