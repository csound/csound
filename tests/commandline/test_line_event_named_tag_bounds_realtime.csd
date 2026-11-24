<CsTest>
description = "under --realtime, a line event's named p1 past the instrument table is refused, here or at dispatch"
args = ["-d", "-odac", "-+rtaudio=null", "--realtime"]

[expect]
exit = 4
stderr = ["instr 201(200) undefined", "instr 1000001(200) undefined",
          "instrument -Hold.1e11 undefined", "instrument Hold.1e11 undefined"]
output = ["Hold plays, p1 1.5"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-m128
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1
0dbfs = 1

; Under realtime locks a line event leaves the table to its dispatch, which
; checks a name's number as a number and deletes the four past the table.
; The two past an int are refused when they are sent: their conversion was
; undefined. "Hold.1e7", 1000001, is exact in a float p-field too.
instr Hold
  prints("Hold plays, p1 %g\n", p1)
endin

instr Send
  scoreline_i({{
i "Hold.2e3" 0 0.1
i "-Hold.1e7" 0 0
d "Hold.1e7" 0 0
d "-Hold.1e11" 0 0
q "Hold.1e11" 0 0
q "-Hold.2e3" 0 0
i "Hold.5" 0 0
}})
endin
</CsInstruments>
<CsScore>
i "Send" 0.1 0
e 0.5
</CsScore>
</CsoundSynthesizer>
