<CsTest>
description = "vtable1k reads complete vectors and follows table changes and note reuse"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
giA ftgen 101, 0, -3, -2, 11, 22, 33
giB ftgen 102, 0, -3, -2, 44, 55, 66
giC ftgen 103, 0, -5, -2, 77, 88, 99, 111, 222
gkDone init 0

instr 1
  tableiw 11, 0, giA
  iFirst table 0, p4
  iSecond table 1, p4
  iThird table 2, p4
  ; Table-number rounding can differ by platform. Use normal table lookup.
  iRoundFirst table 0, 101.75
  iRoundSecond table 1, 101.75
  iRoundThird table 2, 101.75
  kStep init 0
  kTable init p4
  kA init 0
  kB init 0
  kC init 0
  if kStep == 0 then
    kWantA = iFirst
    kWantB = iSecond
    kWantC = iThird
  elseif kStep == 1 then
    kTable = 101.75
    kWantA = iRoundFirst
    kWantB = iRoundSecond
    kWantC = iRoundThird
  elseif kStep == 2 then
    kTable = giC
    kWantA = 77
    kWantB = 88
    kWantC = 99
  elseif kStep == 3 then
    kTable = giA
    kWantA = 11
    kWantB = 22
    kWantC = 33
  elseif kStep == 4 then
    ; Keep the table number unchanged while modifying its data.
    kValue = 77
    tablew kValue, 0, giA
    kWantA = 77
  else
    kTable = giB
    kWantA = 44
    kWantB = 55
    kWantC = 66
  endif
  vtable1k kTable, kA, kB, kC
  if kA != kWantA || kB != kWantB || kC != kWantC then
    printks "vtable1k step %g: got %g %g %g, wanted %g %g %g\n", 0, kStep, kA, kB, kC, kWantA, kWantB, kWantC
    exitnowk -1
  endif
  if kStep == 5 then
    gkDone += 1
    turnoff
  endif
  kStep += 1
endin

instr 99
  if i(gkDone) != 2 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 101.25
i 1 .02 .01 103
i 99 .04 .001
</CsScore>
</CsoundSynthesizer>
