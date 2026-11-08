<CsTest>
description = "quadbezier clipped, fractional, and nearly linear segments"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

; A table may end partway through a segment; retain the curve's coordinates.
giClip ftgen 1, 0, -16, "quadbezier", 0, 16, .5, 32, 1
; Collinear segments must still produce y = x / 16 at integer sample positions.
giFraction ftgen 2, 0, -16, "quadbezier", 0, 2.25, .140625, 4.5, .28125, 10.25, .640625, 16, 1
giNearLine ftgen 3, 0, -16, "quadbezier", 0, 8.0000000001, .5, 16, 1
; Control points may coincide with either endpoint, as the manual permits.
giLeft ftgen 4, 0, -16, "quadbezier", 0, 0, 0, 16, 1
giRight ftgen 5, 0, -16, "quadbezier", 0, 16, 1, 16, 1
giShort ftgen 6, 0, -16, "quadbezier", 0, 4, .5, 8, 1
giCurve ftgen 7, 0, -16, "quadbezier", 0, 8, 1, 16, 0

opcode Check, 0, iii
  iTable, iIndex, iExpected xin
  iValue table iIndex, iTable
  if !(abs(iValue - iExpected) < .0000001) then
    prints "quadbezier table %d index %d: expected %.9f, got %.9f\n", iTable, iIndex, iExpected, iValue
    exitnow -1
  endif
endop

instr 1
  iIndex = 0
  while iIndex < 16 do
    Check giClip, iIndex, iIndex / 32
    Check giFraction, iIndex, iIndex / 16
    Check giNearLine, iIndex, iIndex / 16
    Check giLeft, iIndex, iIndex / 16
    Check giRight, iIndex, iIndex / 16
    Check giCurve, iIndex, 2 * (iIndex / 16) * (1 - iIndex / 16)
    if iIndex <= 8 then
      Check giShort, iIndex, iIndex / 8
    else
      Check giShort, iIndex, 0
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
