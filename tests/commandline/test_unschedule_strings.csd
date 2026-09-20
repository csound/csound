<CsTest>
description = "unschedule distinguishes string and numeric fields and matches multiple strings"
[expect]
exit = 0
stderr = ["string event survived", "numeric event survived"]
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

instr 1
  schedule 2, .1, .01, "first", "second"
  schedule 3, .1, .01, 0
  ; Different p-field types must not match in either direction.
  unschedule 2, .1, .01, 0, "second"
  unscheduleall 3, .1, .01, "first"

  schedule 4, .1, .01, "", "second", "third"
  unschedule 4, .1, .01, "", "second", "third"
  schedule 4, .1, .01, "first", "second"
  schedule 4, .1, .01, "first", "second"
  unscheduleall 4, .1, .01, "first", "second"
endin

instr 2
  prints "string event survived\n"
endin
instr 3
  prints "numeric event survived\n"
endin
instr 4
  exitnow -1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
f 0 .2
</CsScore>
</CsoundSynthesizer>
