<CsTest>
description = "Timed sequence tables must contain sorted events and a complete positive end row"

[expect]
exit = "nonzero"
stderr = ["timedseq: rows need at least an event and time", "timedseq: missing complete end row", "timedseq: invalid sequence end", "timedseq: event times must be sorted and nonnegative"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
giMissing ftgen 0, 0, -4, -2, 1, 0, 2, .5
giTruncated ftgen 0, 0, -3, -2, 1, 0, -1
giZeroEnd ftgen 0, 0, -4, -2, 1, 0, -1, 0
giEmpty ftgen 0, 0, -2, -2, -1, 1
giUnsorted ftgen 0, 0, -6, -2, 1, .5, 2, .25, -1, 1
giNegative ftgen 0, 0, -4, -2, 1, -.25, -1, 1
giEarlyEnd ftgen 0, 0, -4, -2, 1, .75, -1, .5
instr 1
  iTables[] fillarray giMissing, giTruncated, giZeroEnd, giEmpty, giUnsorted, giNegative, giEarlyEnd
  kEvent init 0
  kTime init 0
  kTrig timedseq 0, iTables[p4], kEvent, kTime
endin
instr 2
  kEvent init 0
  kTrig timedseq 0, giMissing, kEvent
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 1
i 1 0 .01 2
i 1 0 .01 3
i 1 0 .01 4
i 1 0 .01 5
i 1 0 .01 6
i 2 0 .01
</CsScore>
</CsoundSynthesizer>
