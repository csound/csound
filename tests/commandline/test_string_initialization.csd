<CsTest>
description = "new strings grow for formatting and retain the getcfg default"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  SNumber = S(42)
  SJoined strcat "hello", " world"
  SFormatted sprintf "%s %d", "answer", 42
  SConfig getcfg 1
  if strcmp(SNumber, "42.000000") != 0 || \
      strcmp(SJoined, "hello world") != 0 || \
      strcmp(SFormatted, "answer 42") != 0 || \
      strcmp(SConfig, "63") != 0 then
    prints "new string output failed\n"
    exitnow -1
  endif

  kValue init 0
  SExpected[] fillarray "0.000000", "1.000000", "2.000000"
  SCurrent = S(kValue)
  if strcmpk(SCurrent, SExpected[kValue]) != 0 then
    printks "numeric string update failed at %g\n", 0, kValue
    exitnowk -1
  endif
  kValue += 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.046875
e
</CsScore>
</CsoundSynthesizer>
