<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 48
nchnls = 1
0dbfs = 1
giSource ftgen 0, 0, 64, 10, 1
giWindow ftgen 0, 0, 32, 20, 2, 1
giExpanded ftgen 0, 0, 64, 2, 0
iIndex = 0
while iIndex < 64 do
  iValue table int(iIndex/2), giWindow
  tableiw iValue, iIndex, giExpanded
  iIndex += 1
od
instr 1
  aInput = .001
  kOffset init 0
  if p4 == 2 then
    aSource oscili .2, 750
    aActual lpcfilter aInput, aSource, 1, 64, 64, 2, giWindow
    aExpected lpcfilter aInput, aSource, 1, 64, 64, 2, giExpanded
  else
    ; Flag zero checks initialization; flag one also checks later analyses.
    aActual lpcfilter aInput, kOffset, p4, giSource, 64, 2, giWindow
    aExpected lpcfilter aInput, kOffset, p4, giSource, 64, 2, giExpanded
  endif
  kPeak init 0
  kCycle init 0
  kIndex = 0
  while kIndex < ksmps do
    kActual vaget kIndex, aActual
    kExpected vaget kIndex, aExpected
    if !(abs(kActual-kExpected) < .00001) then
      printks "lpcfilter window size changed the result, case %g\n", 0, p4
      exitnowk(-1)
    endif
    kPeak = max(kPeak, abs(kActual))
    kIndex += 1
  od
  if kCycle == 99 && !(kPeak > .000001) then
    printks "lpcfilter unexpectedly produced silence\n", 0
    exitnowk(-1)
  endif
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .1 0
i 1 .1 .1 1
i 1 .2 .1 2
</CsScore>
</CsoundSynthesizer>
