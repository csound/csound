<CsoundSynthesizer>
<CsInstruments>
sr=44100
ksmps=1
nchnls=1


opcode genz, i[], 0
  ix[] fillarray 1,2,3,4
  xout ix
endop

;; works for i-rate
instr 1
  iarray[] fillarray 1,2,3,4,5
  for i1 in iarray do
    print i1
  od
endin


;; works for k-rate
instr 2
  karray[] fillarray 1,2,3,4,5
  kdone init 0
  if kdone == 0 then
    kdone = 1
    for k1 in karray do
      printk2 k1
    od
  endif
endin


;; works with inline expression call
instr 3
  for ix in genz() do
    print ix
  od
endin

;; you can get the index of the iterator
instr 4
  for ival, indx in genz() do
    print indx
  od
endin

</CsInstruments>
<CsScore>
i1  0 .1
i2 .1 .1
i3 .1 .1
i4 .1 .1
e
</CsScore>
</CsoundSynthesizer>
