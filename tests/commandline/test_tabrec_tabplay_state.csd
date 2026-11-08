<CsTest>
description = "tabrec and tabplay bind tables, restart, count pulses, and preserve frame boundaries"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
gkDone init 0
giA ftgen 0, 0, -9, -2, 0
giB ftgen 0, 0, -6, -2, 0
giP ftgen 0, 0, -5, -2, 123, 10, 11, 20, 21
giQ ftgen 0, 0, -6, -2, 321, 30, 31, 40, 41, 999

instr 1
  iN = 0
  while iN < 9 do
    tableiw -99, iN, giA
    iN += 1
  od
  iN = 0
  while iN < 6 do
    tableiw -99, iN, giB
    iN += 1
  od
  kStep init 0
  kStopped[] fillarray 1, 100, -100, 101, -101, -99, -99, -99, -99
  kFull[] fillarray 20, 106, -106, 107, -107, 108, -108, 109, -109
  kSwitched[] fillarray 2, 112, -112, 113, -113, -99
  kStart = (kStep == 0 || kStep == 6 || kStep == 12 ? 1 : 0)
  ; Pulses while idle must not affect the next recording.
  kStop = (kStep == 1 || kStep == 4 || kStep == 5 || kStep == 13 ? 1 : 0)
  kTicks = (kStep < 6 ? 1 : (kStep < 12 ? 20 : 2))
  kTable = (kStep < 12 ? giA : giB)
  tabrec kStart, kStop, kTicks, kTable, 100 + kStep, -100 - kStep
  if kStep == 5 || kStep == 11 || kStep == 15 then
    ; Check the stop pulse, same-table restart, and unused partial frame.
    kCheckTable = (kStep == 15 ? giB : giA)
    kLength = (kStep == 15 ? 6 : 9)
    kN = 0
    while kN < kLength do
      if kStep == 5 then
        kWanted = kStopped[kN]
      elseif kStep == 11 then
        kWanted = kFull[kN]
      else
        kWanted = kSwitched[kN]
      endif
      kActual tablekt kN, kCheckTable
      if kActual != kWanted then
        printks "tabrec step %g, index %g: got %g, wanted %g\n", 0, kStep, kN, kActual, kWanted
        exitnowk -1
      endif
      kN += 1
    od
  endif
  if kStep == 15 then
    gkDone += 1
    turnoff
  endif
  kStep += 1
endin

instr 2
  kStep init 0
  kFirst init -9
  kSecond init -8
  kExpected[] fillarray -9, 10, 20, 20, 20, 20, 20, 10, 20, 20, 30, 40, 30, 30, 40
  kTrigger = (kStep == 1 || kStep == 4 || kStep == 7 || kStep == 10 || kStep == 12 || kStep == 13 ? 1 : 0)
  kTable = (kStep < 10 ? giP : giQ)
  kTicks = (kStep < 12 ? 2 : .5)
  tabplay kTrigger, kTicks, kTable, kFirst, kSecond
  if kFirst != kExpected[kStep] || kSecond != kExpected[kStep] + 1 then
    printks "tabplay step %g: got %g, %g, wanted %g\n", 0, kStep, kFirst, kSecond, kExpected[kStep]
    exitnowk -1
  endif
  ; Playback must not overwrite either table's header.
  kP table 0, giP
  kQ table 0, giQ
  kTail table 5, giQ
  if kP != 123 || kQ != 321 || kTail != 999 then
    exitnowk -1
  endif
  if kStep == 14 then
    gkDone += 1
    turnoff
  endif
  kStep += 1
endin

instr 99
  if i(gkDone) != 4 then
    prints "tabrec/tabplay did not finish all checks\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02
i 2 0 .02
; Reuse both opcode instances after they have recorded and played other tables.
i 1 .03 .02
i 2 .03 .02
i 99 .06 .001
</CsScore>
</CsoundSynthesizer>
