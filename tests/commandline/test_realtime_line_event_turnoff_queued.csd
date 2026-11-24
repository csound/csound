<CsTest>
description = "under --realtime, a line event's turnoff in the k-cycle of its note's start turns the note off, whatever is queued after it"
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
; sent with its note and due in the same k-cycle. A turnoff waits for the
; notes queued before it, not for those queued after: the score queues a
; Busy note in every k-cycle for 5 seconds, and must not hold the turnoffs,
; or the events after them, until it stops.
giBusy init 0

instr 1
endin

instr Hold
endin

instr Busy
  giBusy += 1
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

; Check is sent after the four notes and their turnoffs: none plays, and the
; score still queues its Busy notes. It ends the performance.
instr Check
  iPlaying = active:i(1, 0, 1) + active:i("Hold", 0, 1)
  if iPlaying != 0 then
    prints("%d notes still play\n", iPlaying)
    exitnow(1)
  endif
  if giBusy >= 500 then
    prints("the line events waited for the score's last note\n")
    exitnow(1)
  endif
  prints("no note plays\n")
  scoreline_i("e")
endin
</CsInstruments>
<CsScore>
{ 500 N
i "Busy" [0.01 * $N] 0.005
}
i "Send" 0.1 0
e 6
</CsScore>
</CsoundSynthesizer>
