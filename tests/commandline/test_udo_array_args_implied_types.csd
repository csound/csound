<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

;; Test UDO with array args using implied types from variable names
;; This tests the fix for the bug where internal array format [k] was
;; not being converted to external format k[] when concatenated with
;; other args like 'i', resulting in invalid format 'i[k]'

;; UDO with i-rate scalar and k-rate array args (implied from var names)
opcode cycle(indx, kvals[]):k
  kval = kvals[indx % lenarray(kvals)]
  xout kval
endop

;; UDO with i-rate args (implied from var names)
opcode contains(ival, iarr[]):i
  indx = 0
  iret = 0
  while (indx < lenarray:i(iarr)) do
    if (iarr[indx] == ival) then
      iret = 1
      igoto end
    endif
    indx += 1
  od
end:
  xout iret
endop

;; UDO returning array with mixed args
opcode remove(ival, karr[]):k[]
  kout[] init lenarray(karr)
  kout = karr
  xout kout
endop

instr 1
  kArr[] fillarray 1, 2, 3, 4, 5
  iArr[] fillarray 10, 20, 30

  ;; Test cycle UDO
  kVal = cycle(0, kArr)

  ;; Test contains UDO
  iFound = contains(20, iArr)
  iNotFound = contains(99, iArr)

  ;; Test remove UDO
  kResult[] = remove(2, kArr)

  ;; Verify results at init time only
  if (iFound == 1 && iNotFound == 0) then
    prints "PASSED: UDO array args with implied types work correctly\n"
  else
    prints "FAILED: UDO array args test\n"
  endif

  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
