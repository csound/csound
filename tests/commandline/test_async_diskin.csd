<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null --realtime -d -m128
</CsOptions>
<CsInstruments>

gkVictimPerfCycles init 0

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
  ; A combined reader makes two workers release the same owner concurrently.
  ; ASan and TSan make stale instance access easier to reproduce.
  iStart = 0
  while (iStart < 0.2) do
    schedule(1, iStart, 0.02)
    schedule(2, iStart, 0.02)
    schedule(6, iStart, 0.02)
    iStart += 0.002
  od
endin

instr 4
  kReopen metro 500
  if (kReopen == 1) then
    reinit REOPEN
  endif
  kgoto PLAY

REOPEN:
  a1 diskin2 "fox.wav"
  rireturn

PLAY:
  out a1 * 0.1
endin

instr 5
  a1 init 0
  fout "test_async_file.wav", 14, a1
endin

instr 6
  aScalar diskin2 "fox.wav"
  aArray[] diskin2 "fox.wav"
  out (aScalar + aArray[0]) * 0.05
endin

opcode SlowInit():i
  iCount = 0
  while (iCount < 5000000) do
    iCount += 1
  od
  xout iCount
endop

instr 7
  ; Keep trying until instr 8 is visible on the active chain. Its indefinite
  ; duration means zero perf cycles below can only result from this turnoff.
  turnoff2 8, 0, 0
endin

instr 8
  iUnused = SlowInit()
  a1 diskin2 "fox.wav"
  gkVictimPerfCycles += 1
  out a1 * 0.1
endin

instr 9
  if (i(gkVictimPerfCycles) != 0) then
    prints "Cancelled instance reached its performance pass\n"
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i 3 0.1 0
i 4 0 0.05
i 4 0.01 0.05
i 5 0 0.03
i 7 0 0.2
i 8 0 -1
i 9 0.08 0
e 0.4
</CsScore>
</CsoundSynthesizer>
