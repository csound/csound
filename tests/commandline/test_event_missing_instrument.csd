<CsTest>
description = "an event for a missing named instrument is refused, not played as instr 0"

# The score's own note for the missing name is deleted, an error of the
# performance; scoreline_i's is refused as it is inserted, as an invalid
# number is.
[expect]
exit = 1
stderr = ["event: instrument Missing undefined"]
output = ["the global code ran once"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

; A name not found is 0 to named_instr_find, and an event inserted with one
; went on with 0 in its p1: a note for instr 0, which ran the global code
; again - its tables made anew, its globals assigned, its schedules sent.
chnset(chnget:i("runs") + 1, "runs")

instr Send
  scoreline_i({{i "Missing" 0 0.01}})
  event_i("i", "Missing", 0, 0.01)
  schedule("Missing", 0, 0.01)
endin

instr Check
  runs:i = chnget:i("runs")
  if runs != 1 then
    prints("the global code ran %d times\n", runs)
    exitnow(1)
  endif
  prints("the global code ran once\n")
endin
</CsInstruments>
<CsScore>
i "Missing" 0 0.01
i "Send" 0.1 0
i "Check" 0.3 0
</CsScore>
</CsoundSynthesizer>
