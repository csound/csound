<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; UDO intended to set all elements of an array to newval
opcode arrset(arr:k[], newval:k):void
  for val, i in arr do
    arr[i] = newval
  od
endop

; Simple UDO assigning a single element
opcode arrset2(arr:k[], newval:k):void
  arr[0] = newval
endop

; Test arrset: all elements should become newval
instr 1
  arr:k[] = [1 ... 10]
  newval:k = 0.5
  arrset(arr, newval)

  kfail = 0
  for v, i in arr do
    kfail = kfail || (abs(arr[i] - newval) > 1e-6)
  od

  if (kfail != 0) then
    printks "error1\n", 0
    turnoff
  endif
  printks "pass1\n", 0
endin

; Test arrset2: element 0 should become newval
instr 2
  arr:k[] = [1]
  newval:k = 0.75
  arrset2(arr, newval)

  if (abs(arr[0] - newval) > 1e-6) then
    printks "error2\n", 0
    turnoff
  endif
  printks "pass2\n", 0
endin

; Control test without UDO: inline loop assignment should work
instr 3
  arr:k[] = [1 ... 10]
  newval:k = 0.5
  for v, i in arr do
    arr[i] = newval
  od

  kfail = 0
  for v, i in arr do
    kfail = kfail || (abs(arr[i] - newval) > 1e-6)
  od

  if (kfail != 0) then
    printks "error3\n", 0
    turnoff
  endif
  printks "pass3\n", 0
endin

</CsInstruments>
<CsScore>
i1 0 0.01
i2 0.1 0.01
i3 0.2 0.01
</CsScore>
</CsoundSynthesizer>

