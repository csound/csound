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
  for i1 of iarray do
    print i1
  od
endin


;; works for k-rate
instr 2
  karray[] fillarray 1,2,3,4,5
  kdone init 0
  if kdone == 0 then
    kdone = 1
    for k1 of karray do
      printk2 k1
    od
  endif
endin


;; works with inline expression call
instr 3
  for ix of genz() do
    print ix
  od
endin

</CsInstruments>
<CsScore>
i1 0 0.1
i2 + .
i3 + .
e
</CsScore>
</CsoundSynthesizer>
