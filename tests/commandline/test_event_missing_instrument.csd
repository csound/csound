<CsTest>
description = "an event for a missing named instrument is refused, not played as instr 0"

# Each missing name is refused as it is inserted, naming it - a note, a mute
# and a turnoff. A refused insertion is no error of the performance.
[expect]
exit = 0
stderr = ["event: instrument MissingNote undefined",
          "event: instrument MissingMute undefined",
          "event: instrument MissingOff undefined"]
output = ["the global code ran once, and Real twice"]
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
; scorelinei is the way in; eventi and schedule refuse a missing name before
; they insert anything, and the score's own notes are checked as they play.
chnset(chnget:i("runs") + 1, "runs")

instr Real
  chnset(chnget:i("real") + 1, "real")
endin

instr Send
  scorelinei({{i "MissingNote" 0 0.01}})
  scorelinei({{q "MissingMute" 0 1}})
  scorelinei({{d "MissingOff" 0 0}})
  ; A name that is there, and one tagged, still play.
  scorelinei({{i "Real" 0 0.01}})
  scorelinei({{i "Real.1" 0 0.01}})
endin

instr Check
  runs:i = chnget:i("runs")
  real:i = chnget:i("real")
  if runs != 1 || real != 2 then
    prints("the global code ran %d times, and Real %d\n", runs, real)
    exitnow(1)
  endif
  prints("the global code ran once, and Real twice\n")
endin
</CsInstruments>
<CsScore>
i "Send" 0 0
i "Check" 0.3 0
</CsScore>
</CsoundSynthesizer>
