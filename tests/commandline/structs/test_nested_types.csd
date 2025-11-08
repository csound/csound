<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

; Test nested struct type definitions - avoid mixed init for now
struct Point x:i, y:i

instr 1
  ; Test basic struct member access first
  point1:Point init 10, 20

  ; Test simple member access (should work)
  ix = point1.x
  iy = point1.y
  prints "point1.x = %d, point1.y = %d\n", ix, iy

  if (ix != 10 || iy != 20) then
    prints "Basic struct test failed!\n"
    exitnow 1
  endif

  prints "Basic struct test passed!\n"
endin

instr 2
  ; Test nested member assignment (this is what we want to support)
  point2:Point init 5, 15

  ; Test member assignments
  point2.x = 100
  point2.y = 200

  ix = point2.x
  iy = point2.y
  prints "After assignment: point2.x = %d, point2.y = %d\n", ix, iy

  if (ix != 100 || iy != 200) then
    prints "Member assignment test failed!\n"
    exitnow 1
  endif

  prints "Member assignment test passed!\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.2 0.1
e
</CsScore>
</CsoundSynthesizer>
