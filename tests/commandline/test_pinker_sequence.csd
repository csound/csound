<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 8192
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 ; Samples from the original sequence rendered with ksmps=16. A larger
 ; block must preserve that sequence across each 256-entry mask cycle.
 iPositions[] fillarray 0, 1, 7, 15, 4095, 4096, 8191, 8192, 12288, 16383
 iValues[] fillarray -.06201171875, .06243896484375, -.03680419921875, .038116455078125, .0836181640625, .191741943359375, .11297607421875, .042724609375, .333099365234375, .285491943359375
 aOut pinker
 kBase init 0
 kN = 0
 while kN < lenarray(iPositions) do
  kIndex = iPositions[kN]-kBase
  if kIndex >= 0 && kIndex < ksmps then
   kActual vaget kIndex, aOut
   if !(abs(kActual-iValues[kN]) < .00000001) then
    printks "pinker sequence sample=%g actual=%g expected=%g\n", 0, iPositions[kN], kActual, iValues[kN]
    exitnowk(-1)
   endif
  endif
  kN += 1
 od
 kBase += ksmps
 if kBase == 16384 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 1 then
  prints "pinker checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 2
i 99 3 .001
e
</CsScore>
</CsoundSynthesizer>
