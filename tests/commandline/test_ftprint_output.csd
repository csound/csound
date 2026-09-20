<CsTest>
description = "ftprint handles long rows and bounded table slices"
[expect]
exit = 0
stderr_regex = [' 0: (?:0\.0000 ){1023}0\.0000\n', ' 2: 3\.0000\n   3: 4\.0000\n', ' 1: 2\.0000 \n']
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr 1
  iLong ftgen 0, 0, 1024, -2, 0
  ftprint iLong, 1, 0, 0, 1, 1024
  iShort ftgen 0, 0, -4, -2, 1, 2, 3, 4
  ftprint iShort, 1, -3, -1, 1, 1
  ftprint iShort, 1, 1, 0, 4294967040
endin
</CsInstruments>
<CsScore>
i 1 0 .015625
e
</CsScore>
</CsoundSynthesizer>
