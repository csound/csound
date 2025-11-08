<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

; Test struct-to-struct references (the core requirement)
struct Point x:i, y:i
struct Rectangle topLeft:Point, width:i, height:i

instr 1
  ; Test struct with struct member (struct-to-struct reference)
  point1:Point init 10, 20

  ; First test that point1 is initialized correctly
  iPointX = point1.x
  iPointY = point1.y
  prints "point1.x=%d\n", iPointX
  prints "point1.y=%d\n", iPointY

  rect1:Rectangle init point1, 100, 50

  ; Test accessing nested struct members
  iX = rect1.topLeft.x
  iY = rect1.topLeft.y
  iWidth = rect1.width
  iHeight = rect1.height

  prints "rect1.topLeft.x=%d\n", iX
  prints "rect1.topLeft.y=%d\n", iY
  prints "rect1.width=%d\n", iWidth
  prints "rect1.height=%d\n", iHeight

endin

</CsInstruments>
<CsScore>
i1 0 0.

</CsScore>
</CsoundSynthesizer>
