<CsTest>
description = "test diskin in rt async mode"
args = []

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null --realtime -d -m128
</CsOptions>
<CsInstruments>

gkVictimPerfCycles init 0
gkVictimInitCompleted init 0
gkTurnoffAttempts init 0
gkUdoFirst init 0
gkUdoSecond init 0
gkSubFirst init 0
gkSubSecond init 0
; Keep separate records for the first note and the reused note.
gkUdoInitStarted[] init 2
gkUdoInitFinished[] init 2
gkUdoPerfCycles[] init 2

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
  gkTurnoffAttempts += 1
  turnoff2 8, 0, 0
endin

instr 8
  iUnused = SlowInit()
  a1 diskin2 "fox.wav"
  ; Publish completion from the init thread before this cancelled instance
  ; could enter its performance pass.
  gkVictimInitCompleted init 1
  gkVictimPerfCycles += 1
  out a1 * 0.1
endin

instr 9
  ; This watcher starts before instr 8, so SlowInit cannot delay the assertion
  ; itself. Keep process termination on the performance thread: exitnow would
  ; otherwise longjmp from the realtime init thread on a failed assertion.
  if (gkVictimInitCompleted != 0) then
    if (gkVictimPerfCycles != 0) then
      printks "Cancelled instance reached its performance pass\n", 0
      exitnowk(1)
    endif
    if (gkTurnoffAttempts == 0) then
      printks "Cancellation test made no turnoff attempts\n", 0
      exitnowk(1)
    endif
    exitnowk(0)
  ; Leave a wide margin for slow debug and sanitizer builds.
  elseif (timeinsts() > 30.0) then
    printks "Cancellation test did not complete its init pass\n", 0
    exitnowk(1)
  endif
endin

opcode NestedDiskin():a
  audio:a diskin2 "fox.wav"
  xout audio
endop

instr 11
  iNote = p4 - 1
  gkUdoInitStarted[iNote] init 1
  audio:a = NestedDiskin()
  gkUdoInitFinished[iNote] init 1
  gkUdoPerfCycles[iNote] += 1
  level:k rms audio
  if (p4 == 1) then
    gkUdoFirst = max(gkUdoFirst, level)
  else
    gkUdoSecond = max(gkUdoSecond, level)
  endif
  if (level > 0) then
    turnoff
  endif
endin

instr 12
  audio:a diskin2 "fox.wav"
  out audio * 0.1
endin

instr 13
  audio:a subinstr 12
  level:k rms audio
  if (p4 == 1) then
    gkSubFirst = max(gkSubFirst, level)
  else
    gkSubSecond = max(gkSubSecond, level)
  endif
  if (level > 0) then
    turnoff
  endif
endin

instr 10
  ; Wait for each reader to produce audio and turn off before reusing it.
  ; Fixed score times can reach the check before async init has caught up.
  kStage init 0
  if (kStage == 0) then
    schedulek(11, 0, -1, 1)
    kStage = 1
  elseif (kStage == 1 && gkUdoFirst > 0) then
    schedulek(11, 0, -1, 2)
    kStage = 2
  elseif (kStage == 2 && gkUdoSecond > 0) then
    schedulek(13, 0, -1, 1)
    kStage = 3
  elseif (kStage == 3 && gkSubFirst > 0) then
    schedulek(13, 0, -1, 2)
    kStage = 4
  elseif (kStage == 4 && gkSubSecond > 0) then
    ; Complete overlap, reinit, and file-close stress before blocking init.
    schedulek(3, 0.05, 0)
    schedulek(4, 0.05, 0.05)
    schedulek(4, 0.06, 0.05)
    schedulek(5, 0.05, 0.03)
    ; Start the turnoff loop and watcher before the victim blocks init.
    schedulek(7, 0.35, 30.5)
    schedulek(9, 0.35, 30.5)
    schedulek(8, 0.40, -1)
    turnoff
  endif

  if (timeinsts() > 30.0) then
    if (gkUdoFirst <= 0 || gkUdoSecond <= 0) then
      ; A zero level alone cannot tell a silent reader from a note that never
      ; ran. Report both notes before exiting so CI preserves that distinction.
      printks "UDO first note: init entered=%d completed=%d perf cycles=%d\n", \
        0, gkUdoInitStarted[0], gkUdoInitFinished[0], gkUdoPerfCycles[0]
      printks "UDO second note: init entered=%d completed=%d perf cycles=%d\n", \
        0, gkUdoInitStarted[1], gkUdoInitFinished[1], gkUdoPerfCycles[1]
      printks "Realtime UDO diskin2 reuse failed: first=%f second=%f\n", \
        0, gkUdoFirst, gkUdoSecond
      exitnowk(1)
    endif
    if (gkSubFirst <= 0 || gkSubSecond <= 0) then
      printks "Realtime subinstr diskin2 reuse failed: first=%f second=%f\n", \
        0, gkSubFirst, gkSubSecond
      exitnowk(1)
    endif
    turnoff
  endif
endin

</CsInstruments>
<CsScore>
i 10 0 -1
e 62
</CsScore>
</CsoundSynthesizer>
