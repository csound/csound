<CsTest>
description = "exitnow at init under --realtime ends the performance with its value"
args = ["-nd", "--realtime"]

[expect]
exit = 3
stderr = ["realtime mode enabled"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1

; The init thread runs this. Its jump used to land in the frames of the
; thread that performs, and the csound command died of a segmentation fault.
instr 1
  exitnow 3
endin
</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
