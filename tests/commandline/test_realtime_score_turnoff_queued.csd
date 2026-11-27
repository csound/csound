<CsTest>
description = "under --realtime, a score turnoff in the k-cycle of its note's start turns the note off"
args = ["-nd", "--realtime"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1
0dbfs = 1

; Under --realtime a score note is inserted on the allocation thread, and
; a turnoff read in the same k-cycle looked for it before it was there: it
; found nothing, and the note played on. Each turnoff below comes 1 ms after
; its note, in the same k-cycle.
instr 1
endin

instr Hold
endin

; Check is inserted after the four notes, by the same queue: none plays.
instr Check
  iPlaying = active:i(1, 0, 1) + active:i("Hold", 0, 1)
  if iPlaying != 0 then
    prints("%d notes still play\n", iPlaying)
    exitnow(1)
  endif
endin
</CsInstruments>
<CsScore>
i 1.1 0.1 -1 1
d 1.1 0.101 0
i 1.2 0.1 -1 2
i -1.2 0.101 0
i "Hold.3" 0.1 -1 3
d "Hold.3" 0.101 0
i "Hold.4" 0.1 -1 4
i "-Hold.4" 0.101 0
i "Check" 0.3 0
e 0.4
</CsScore>
</CsoundSynthesizer>
