<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null --realtime -d -m128
</CsOptions>
<CsInstruments>

instr 1
  a1 diskin2 "fox.wav"
  out a1 * 0.1
endin

instr 2
  a1[] diskin2 "fox.wav"
  out a1[0] * 0.1
endin

instr 3
  ; Short overlapping notes race asynchronous init, deinit, and shutdown.
  ; ASan and TSan make stale instance access easier to reproduce.
  iStart = 0
  while (iStart < 0.2) do
    schedule(1, iStart, 0.02)
    schedule(2, iStart, 0.02)
    iStart += 0.002
  od
endin

</CsInstruments>
<CsScore>
i 3 0 0
e 0.25
</CsScore>
</CsoundSynthesizer>
