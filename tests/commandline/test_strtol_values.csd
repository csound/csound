<CsTest>
description = "strtol forms share signed decimal, octal and hexadecimal parsing"
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
giChecks init 0
gkChecks init 0
instr 1
  SValue strget p4
  strset 100, SValue
  iString strtol SValue
  iPfield strtol p4
  iSet strtol 100
  if iString != p5 || iPfield != p5 || iSet != p5 then
    prints "strtol forms disagree with the expected value\n"
    exitnow -1
  endif
  giChecks += 1
endin
instr 2
  SValues[] fillarray "0", "-0", "+42", "077", "-077", "0xff", "-0Xff", "2147483647", "-2147483648", "017777777777", "-020000000000", "0x7fffffff", "-0x80000000"
  iValues[] fillarray 0, 0, 42, 63, -63, 255, -255, 2147483647, -2147483648, 2147483647, -2147483648, 2147483647, -2147483648
  kIndex init 0
  SValue init "0"
  igoto convert
  SValue strcpyk SValues[kIndex]
convert:
  kValue strtolk SValue
  if kValue != iValues[kIndex] then
    printks "strtolk case %g: got %g\n", 0, kIndex, kValue
    exitnowk -1
  endif
  kIndex += 1
  if kIndex == lenarray(iValues) then
    gkChecks += 1
    turnoff
  endif
endin
instr 99
  if giChecks != 14 || i(gkChecks) != 1 then
    prints "strtol checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .001 "0" 0
i1 0 .001 "-0" 0
i1 0 .001 "+42" 42
i1 0 .001 "   +42" 42
i1 0 .001 "077" 63
i1 0 .001 "-077" -63
i1 0 .001 "0xff" 255
i1 0 .001 "-0Xff" -255
i1 0 .001 "2147483647" 2147483647
i1 0 .001 "-2147483648" -2147483648
i1 0 .001 "017777777777" 2147483647
i1 0 .001 "-020000000000" -2147483648
i1 0 .001 "0x7fffffff" 2147483647
i1 0 .001 "-0x80000000" -2147483648
i2 .01 .05
i99 .1 .001
</CsScore>
</CsoundSynthesizer>
