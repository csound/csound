<CsTest>
description = "native vector results select vector UDO overloads at init and performance"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

; A wrong or unknown rank must not silently choose the vector overload.
opcode Choose(v:i[]):i
  result:i = 1
  xout result
endop
opcode Choose(v:i[][]):i
  result:i = 2
  xout result
endop
opcode Choose(v:k[]):k
  result:k = 1
  xout result
endop
opcode Choose(v:k[][]):k
  result:k = 2
  xout result
endop
; The default control-rate FFT overloads return complex vectors.
opcode Choose(v:Complex[]):k
  result:k = 1
  xout result
endop
opcode Choose(v:Complex[][]):k
  result:k = 2
  xout result
endop
opcode CheckControl(choice:k):void
  if choice != 1 then
    printks "native vector result selected the wrong rank\n", 0
    exitnowk(-1)
  endif
endop
instr 1
  ; All these native operations return vectors without a result annotation.
  v:i[] fillarray 1, 2, 3, 4
  assertEquals(Choose(sorta(v)), 1)
  assertEquals(Choose(sortd(v)), 1)
  assertEquals(Choose(slicearray(v, 0, 3)), 1)
  assertEquals(Choose(limit(v, 0, 2)), 1)
  assertEquals(Choose(abs(v)), 1)
  assertEquals(Choose(sqrt(v)), 1)
  assertEquals(Choose(floor(v)), 1)
  assertEquals(Choose(cos(v)), 1)
  assertEquals(Choose(log(v)), 1)
  assertEquals(Choose(pow(v, 2)), 1)
  assertEquals(Choose(pow(v, v)), 1)
  assertEquals(Choose(interleave(v, v)), 1)
  assertEquals(Choose(rfft(v)), 1)
  assertEquals(Choose(fft(v)), 1)
  assertEquals(Choose(dct(v)), 1)
  assertEquals(Choose(maparray(v, "sqrt")), 1)
  ; Check control-rate expressions too.
  controls:k[] fillarray 1, 2, 3, 4
  CheckControl(Choose(sorta(controls)))
  CheckControl(Choose(sortd(controls)))
  CheckControl(Choose(slicearray(controls, 0, 3)))
  CheckControl(Choose(limit(controls, 0, 2)))
  CheckControl(Choose(abs(controls)))
  CheckControl(Choose(sqrt(controls)))
  CheckControl(Choose(floor(controls)))
  CheckControl(Choose(cos(controls)))
  CheckControl(Choose(log(controls)))
  CheckControl(Choose(pow(controls, 2)))
  CheckControl(Choose(pow(controls, controls)))
  CheckControl(Choose(rfft(controls)))
  CheckControl(Choose(fft(controls)))
  CheckControl(Choose(dct(controls)))
  CheckControl(Choose(maparray(controls, "sqrt")))
  CheckControl(Choose(interleave(controls, controls)))
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
