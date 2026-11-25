<CsTest>
description = "under --realtime, a turnoff finds its note through the note's reinit"
args = ["-d", "-odac", "-+rtaudio=null", "--realtime"]

[expect]
exit = 0
output = ["neither note plays"]
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

; Hold's turnoff comes at 0.1 with an unrelated Slow init: it waited for
; Slow, Hold asked for a reinit at 0.12, and the turnoff, let go when Slow
; ended, found Hold in it and missed it. Hold2's turnoff comes at 0.15,
; while Hold2's reinit, asked for at 0.1, runs: it waits for the reinit.
; Each reinit runs for 0.2 s of the performance, as Slow's init does: Clock
; gives the performance's time to an init.
gkNow init 0
giHoldPasses init 0
giHold2Passes init 0

instr Clock
  gkNow = times:k()
endin

opcode Spin(seconds:i):void
  iStart = i(gkNow)
  iSpin = 0
  while i(gkNow) < iStart + seconds && iSpin < 1e9 do
    iSpin += 1
  od
endop

instr Slow
  Spin(0.2)
endin

instr Hold
  kAsked init 0
  if timeinsts:k() >= 0.12 && kAsked == 0 then
    kAsked = 1
    reinit Again
  endif
Again:
  giHoldPasses += 1
  if giHoldPasses > 1 then
    Spin(0.2)
  endif
  rireturn
endin

instr Hold2
  kAsked init 0
  if timeinsts:k() >= 0.05 && kAsked == 0 then
    kAsked = 1
    reinit Again
  endif
Again:
  giHold2Passes += 1
  if giHold2Passes > 1 then
    Spin(0.2)
  endif
  rireturn
endin

; Check is read after both turnoffs.
instr Check
  iPlaying = active:i("Hold", 0, 1) + active:i("Hold2", 0, 1)
  if iPlaying != 0 then
    prints("%d notes still play\n", iPlaying)
    exitnow(1)
  endif
  prints("neither note plays\n")
  scoreline_i("e")
endin
</CsInstruments>
<CsScore>
i "Clock" 0 -1
i "Hold" 0 -1
i "Hold2" 0.05 -1
i "Slow" 0.1 0
d "Hold" 0.1 0
i "-Hold2" 0.15 0
i "Check" 0.8 0
e 5
</CsScore>
</CsoundSynthesizer>
