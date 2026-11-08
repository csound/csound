<CsTest>
description = "loscilx rejects a table shorter than one multichannel frame"

[expect]
exit = "nonzero"
stderr = ["loscilx: table contains no complete frames"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsFileB filename="loscilx-stereo.wav">
UklGRigAAABXQVZFZm10IBAAAAABAAIAAAQAABAAAAAEABAAZGF0YQQAAAAAAQAC
</CsFileB>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
; The table has only one sample, but the file has two channels.
giTable ftgen 0, 0, 1, -1, "loscilx-stereo.wav", 0, 0, 0
instr 1
  aLeft, aRight loscilx 1, 1, giTable, 1, 1, 0, 1
endin
instr 2
  aChannels[] loscilx 1, 1, giTable, 1, 1, 0, 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
</CsScore>
</CsoundSynthesizer>
