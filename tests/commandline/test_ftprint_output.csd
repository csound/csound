<CsTest>
description = "ftprint handles long rows, bounded table slices and a held trigger"
[expect]
exit = 0
output_regex = [
  ' 0: (?:0\.0000 ){1023}0\.0000\r?\n',
  ' 2: 3\.0000\r?\n   3: 4\.0000\r?\n',
  ' 1: 2\.0000 \r?\n',
  ' 0: 1\.0000 2\.0000 3\.0000\r?\n   3: 4\.0000 5\.0000 \r?\n',
]
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
  ; One row longer than both the old row buffer and the host message buffer.
  iLong ftgen 0, 0, 1024, -2, 0
  ftprint iLong, 1, 0, 0, 1, 1024

  ; Negative indices select the last two values.
  iShort ftgen 0, 0, -4, -2, 1, 2, 3, 4
  ftprint iShort, 1, -3, -1, 1, 1

  ; A large step must stop after one value instead of wrapping the index.
  ftprint iShort, 1, 1, 0, 4294967040
endin

; Raise the trigger at performance time. Holding it high must print only once.
instr 2
  iValues ftgen 0, 0, -5, -2, 1, 2, 3, 4, 5
  kTrigger init 0
  kTrigger = 1
  ftprint iValues, kTrigger, 0, 0, 1, 3
endin
</CsInstruments>
<CsScore>
i 1 0 .015625
i 2 .03125 .03125
e
</CsScore>
</CsoundSynthesizer>
