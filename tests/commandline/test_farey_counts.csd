<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
giRef102 ftgen 102, 0, -2, -2, 0, 1
giRef103 ftgen 103, 0, -2, -2, 1, 1
giRef106 ftgen 106, 0, -5, -2, 0, 1, 1, 2, 1
giRef107 ftgen 107, 0, -5, -2, 1, 3, 2, 3, 1
giRef110 ftgen 110, 0, -11, -2, 0, 1, 1, 1, 2, 1, 3, 2, 3, 4, 1
giRef111 ftgen 111, 0, -11, -2, 1, 5, 4, 3, 5, 2, 5, 3, 4, 5, 1
instr 1
 iLength fareyleni p4
 kLength fareylen p4
 if iLength != p5 then
  prints "fareyleni(%g)=%g expected=%g\n", p4, iLength, p5
  exitnow(-1)
 endif
 if kLength != p5 then
  printks "fareylen(%g)=%g expected=%g\n", 0, p4, kLength, p5
  exitnowk(-1)
 endif
endin

; Check every output mode, including truncation and zero padding.
instr 2
 iTable ftgen 0, 0, -p6, "farey", p4, p5
 iNumerators = 100 + p4*2
 iDenominators = iNumerators + 1
 iLength = ftlen(iNumerators)
 iIndex = 0
 while iIndex < p6 do
  iExpected = 0
  if p5 == 1 then
   if iIndex < iLength-1 then
    iP table iIndex, iNumerators
    iQ table iIndex, iDenominators
    iNextP table iIndex+1, iNumerators
    iNextQ table iIndex+1, iDenominators
    iExpected = iNextP/iNextQ - iP/iQ
   endif
  elseif iIndex < iLength then
   iP table iIndex, iNumerators
   iQ table iIndex, iDenominators
   if p5 == 0 then
    iExpected = iP/iQ
   elseif p5 == 2 then
    iExpected = iQ
   elseif p5 == 3 then
    iExpected = iQ/p4
   else
    iExpected = 1+iP/iQ
   endif
  endif
  iActual table iIndex, iTable
  if !(abs(iActual-iExpected) < .00001) then
   prints "GENfarey order=%g mode=%g index=%g: got=%g expected=%g\n", p4, p5, iIndex, iActual, iExpected
   exitnow(-1)
  endif
  iIndex += 1
 od
endin

; Check a prime beyond the old fixed prime list.
instr 3
 iTable ftgen 0, 0, -p5, "farey", p4, 0
 iFirst table 1, iTable
 iBeforeLast table p5-2, iTable
 iLast table p5-1, iTable
 if !(abs(iFirst-1/p4) < .00001) || !(abs(iBeforeLast-(p4-1)/p4) < .00001) || iLast != 1 then
  prints "GENfarey endpoint or count is wrong for order %g\n", p4
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001 1 2
i 1 0 .001 2 3
i 1 0 .001 3 5
i 1 0 .001 4 7
i 1 0 .001 5 11
i 1 0 .001 6 13
i 1 0 .001 7 19
i 1 0 .001 8 23
i 1 0 .001 9 29
i 1 0 .001 10 33
i 1 0 .001 20 129
i 1 0 .001 100 3045
i 1 0 .001 1009 309935
i 1 0 .001 10007 30443915
i 2 0 .001 1 0 1
i 2 0 .001 1 0 2
i 2 0 .001 1 0 5
i 2 0 .001 1 1 1
i 2 0 .001 1 1 2
i 2 0 .001 1 1 5
i 2 0 .001 1 2 1
i 2 0 .001 1 2 2
i 2 0 .001 1 2 5
i 2 0 .001 1 3 1
i 2 0 .001 1 3 2
i 2 0 .001 1 3 5
i 2 0 .001 1 4 1
i 2 0 .001 1 4 2
i 2 0 .001 1 4 5
i 2 0 .001 3 0 1
i 2 0 .001 3 0 5
i 2 0 .001 3 0 8
i 2 0 .001 3 1 1
i 2 0 .001 3 1 5
i 2 0 .001 3 1 8
i 2 0 .001 3 2 1
i 2 0 .001 3 2 5
i 2 0 .001 3 2 8
i 2 0 .001 3 3 1
i 2 0 .001 3 3 5
i 2 0 .001 3 3 8
i 2 0 .001 3 4 1
i 2 0 .001 3 4 5
i 2 0 .001 3 4 8
i 2 0 .001 5 0 1
i 2 0 .001 5 0 11
i 2 0 .001 5 0 14
i 2 0 .001 5 1 1
i 2 0 .001 5 1 11
i 2 0 .001 5 1 14
i 2 0 .001 5 2 1
i 2 0 .001 5 2 11
i 2 0 .001 5 2 14
i 2 0 .001 5 3 1
i 2 0 .001 5 3 11
i 2 0 .001 5 3 14
i 2 0 .001 5 4 1
i 2 0 .001 5 4 11
i 2 0 .001 5 4 14
i 3 0 .001 1009 309935
e
</CsScore>
</CsoundSynthesizer>

