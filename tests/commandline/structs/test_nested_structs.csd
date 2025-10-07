<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

; Define basic struct types (nested struct types may not be supported yet)
struct Point x:i, y:i
struct Rectangle width:i, height:i, x:i, y:i

instr 1
  ; Test simple member assignment (should work)
  point1:Point init 10, 20
  point1.x = 42

  ix = point1.x
  if (ix != 42) then
    prints "Simple Point assignment failed: expected 42, got %d\n", ix
    exitnow 1
  endif

  prints "Simple Point assignment test passed\n"
endin

instr 2
  ; Test multiple member assignments on same struct
  rect1:Rectangle init 10, 20, 30, 40

  prints "Before assignments: rect1.width=%d, rect1.height=%d, rect1.x=%d, rect1.y=%d\n", rect1.width, rect1.height, rect1.x, rect1.y

  ; Multiple member assignments
  rect1.width = 100
  rect1.height = 200
  rect1.x = 50
  rect1.y = 75

  prints "After assignments: rect1.width=%d, rect1.height=%d, rect1.x=%d, rect1.y=%d\n", rect1.width, rect1.height, rect1.x, rect1.y

  ; Check all assignments
  if (rect1.width != 100 || rect1.height != 200 || rect1.x != 50 || rect1.y != 75) then
    prints "Multiple member assignments failed\n"
    exitnow 1
  endif

  prints "Multiple member assignments test passed\n"
endin

instr 3
  ; Test edge case: assignment to same member multiple times
  point2:Point init 1, 2

  prints "Initial: point2.x=%d, point2.y=%d\n", point2.x, point2.y

  ; Multiple assignments to same member
  point2.x = 10
  point2.x = 20
  point2.x = 30

  prints "After multiple x assignments: point2.x=%d\n", point2.x

  if (point2.x != 30) then
    prints "Multiple assignments to same member failed: expected 30, got %d\n", point2.x
    exitnow 1
  endif

  prints "Multiple assignments to same member test passed\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0.2 0.1
i 3 0.4 0.1
e
</CsScore>
</CsoundSynthesizer>
