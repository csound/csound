<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct Point x:i, y:i

instr 1
  ; Test struct-to-struct assignment with same type
  var1:Point init 1, 2
  var2:Point = var1  ; This should copy values from var1 to var2

  ; Print initial values to verify copying
  printks "var1 initial: x=%f, y=%f\n", 0, var1.x, var1.y
  printks "var2 initial: x=%f, y=%f\n", 0, var2.x, var2.y

  ; Check if values were copied correctly
  if (var1.x == var2.x && var1.y == var2.y) then
    printks "SUCCESS: Struct assignment copying works correctly!\n", 0
  else
    printks "FAILURE: Struct assignment copying failed!\n", 0
  endif
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
