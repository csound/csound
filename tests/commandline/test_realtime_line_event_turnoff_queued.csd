<CsTest>
description = "under --realtime, a line event's turnoff in the k-cycle of its note's start turns the note off"
args = ["-d", "-odac", "-+rtaudio=null", "--realtime"]

[expect]
exit = 0
output = ["no note plays"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-m128 -b 10 -B 20
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1
0dbfs = 1

; Under --realtime a note is inserted on the allocation thread, and a line
; event's turnoff dispatched in the same k-cycle looked for it before it was
; there: it found nothing, and the note played on. Each turnoff below is
; sent with its note and due in the same k-cycle.
instr 1
endin

instr Hold
endin

instr Send
  scoreline_i({{
i 1.1 0 -1
d 1.1 0 0
i 1.2 0 -1
i -1.2 0 0
i "Hold.3" 0 -1
d "Hold.3" 0 0
i "Hold.4" 0 -1
i "-Hold.4" 0 0
i "Check" 0.1 0
}})
endin

; Check is sent after the four notes and their turnoffs: none plays.
instr Check
  iPlaying = active:i(1, 0, 1) + active:i("Hold", 0, 1)
  if iPlaying != 0 then
    prints("%d notes still play\n", iPlaying)
    exitnow(1)
  endif
  prints("no note plays\n")
endin
</CsInstruments>
<CsScore>
i "Send" 0.1 0
e 1
</CsScore>
</CsoundSynthesizer>
