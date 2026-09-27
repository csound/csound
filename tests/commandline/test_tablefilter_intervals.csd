<CsTest>
description = "tablefilter interval mode gives the same result in place and in a separate table"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

; Each source is followed by its expected intervals. Mode 3 skips zero onsets.
giEven ftgen 101, 0, -4, -2, .25, .5, .75, 1
giEvenIntervals ftgen 201, 0, -4, -2, .25, .25, .25, .25
giUneven ftgen 102, 0, -4, -2, .125, .375, .5, 1
giUnevenIntervals ftgen 202, 0, -4, -2, .125, .25, .125, .5
giGaps ftgen 103, 0, -8, -2, .25, .5, 0, .75, 0, 1, 0, 0
giGapIntervals ftgen 203, 0, -4, -2, .25, .25, .25, .25
giRepeated ftgen 104, 0, -8, -2, 0, .25, .25, 0, .5, .75, 1, 0
giRepeatedIntervals ftgen 204, 0, -5, -2, .25, 0, .25, .25, .25
giEmpty ftgen 105, 0, -4, -2, 0
giNoIntervals ftgen 205, 0, -4, -2, 0

instr 1
 iSource = p4
 iExpected = p5
 iExpectedCount = p6
 iSize = ftlen(iSource)
 iInitSeparate ftgen 0, 0, -iSize, -2, 0
 iInitInPlace ftgen 0, 0, -iSize, -2, 0
 iPerfSeparate ftgen 0, 0, -iSize, -2, 0
 iPerfInPlace ftgen 0, 0, -iSize, -2, 0

 tableicopy iInitInPlace, iSource
 iSeparateCount tablefilteri iInitSeparate, iSource, 3, 1
 iInPlaceCount tablefilteri iInitInPlace, iInitInPlace, 3, 1
 if iSeparateCount != iExpectedCount || iInPlaceCount != iExpectedCount then
  prints "tablefilteri source %g: separate count=%g, in-place count=%g, expected=%g\n", iSource, iSeparateCount, iInPlaceCount, iExpectedCount
  exitnow(-1)
 endif

 ; Restore the onsets each control cycle before converting them in place.
 tablecopy iPerfInPlace, iSource
 kSeparateCount tablefilter iPerfSeparate, iSource, 3, 1
 kInPlaceCount tablefilter iPerfInPlace, iPerfInPlace, 3, 1
 if kSeparateCount != iExpectedCount || kInPlaceCount != iExpectedCount then
  printks "tablefilter source %g: separate count=%g, in-place count=%g, expected=%g\n", 0, iSource, kSeparateCount, kInPlaceCount, iExpectedCount
  exitnowk(-1)
 endif

 kIndex = 0
 while kIndex < iExpectedCount do
  kExpected table kIndex, iExpected
  kInitSeparate table kIndex, iInitSeparate
  kInitInPlace table kIndex, iInitInPlace
  kPerfSeparate table kIndex, iPerfSeparate
  kPerfInPlace table kIndex, iPerfInPlace
  if kInitSeparate != kExpected || kInitInPlace != kExpected || kPerfSeparate != kExpected || kPerfInPlace != kExpected then
   printks "source %g, interval %g: init separate=%g in-place=%g; perf separate=%g in-place=%g; expected=%g\n", 0, iSource, kIndex, kInitSeparate, kInitInPlace, kPerfSeparate, kPerfInPlace, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
endin
</CsInstruments>
<CsScore>
; source, expected intervals, number of intervals
i 1 0.00 .004 101 201 4
i 1 0.01 .004 102 202 4
i 1 0.02 .004 103 203 4
i 1 0.03 .004 104 204 5
i 1 0.04 .004 105 205 0
e
</CsScore>
</CsoundSynthesizer>
