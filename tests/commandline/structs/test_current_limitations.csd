<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

; Define basic struct types
struct Point x:i, y:i
struct MyComplex real:i, imag:i

instr 1
  ; Test what our current implementation handles well
  point1:Point init 10, 20
  complex1:MyComplex init 5, 15

  ; These work perfectly with our current ##member_set expansion
  point1.x = 100
  point1.y = 200
  complex1.real = 300
  complex1.imag = 400

  prints "Current implementation works great for:\n"
  prints "  point1.x = %d\n", point1.x
  prints "  point1.y = %d\n", point1.y
  prints "  complex1.real = %d\n", complex1.real
  prints "  complex1.imag = %d\n", complex1.imag
endin


</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
