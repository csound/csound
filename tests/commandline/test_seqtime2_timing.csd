<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1

giZero ftgen 0, 0, 4, -2, 0, .5, .25, .75
giDelay ftgen 0, 0, 4, -2, .25, .5, .125, .5
giReverse ftgen 0, 0, 4, -2, 0, .125, .25, .5
giPrecise ftgen 0, 0, 2, -2, 0, .5000000001
giEmptyIntervals ftgen 0, 0, 4, -2, 0, 0, .25, .125

instr 1
  kCycle init 0
  kCycle += 1
  kExpected = 0
  if p4 == 1 then
    ; Zero delay: the second control cycle must not trigger again.
    kOut seqtime2 0, 1, 0, 4, 0, giZero
    if kCycle == 1 then
      kExpected = .5
    elseif kCycle == 65 then
      kExpected = .25
    elseif kCycle == 97 then
      kExpected = .75
    endif
  elseif p4 == 2 then
    ; Initial delay and event values both use the time unit.
    kOut seqtime2 0, 2, 0, 4, 0, giDelay
    if kCycle == 65 then
      kExpected = 1
    elseif kCycle == 193 then
      kExpected = .25
    endif
  elseif p4 == 3 then
    ; Reverse range [1, 4) must never visit element zero.
    kOut seqtime2 0, 1, 1, -4, 1, giReverse
    if kCycle == 17 || kCycle == 129 then
      kExpected = .5
    elseif kCycle == 81 then
      kExpected = .25
    elseif kCycle == 113 then
      kExpected = .125
    endif
  elseif p4 == 4 then
    ; One-shot includes its endpoint, then stays silent until reset.
    kReset = (kCycle == 161 ? 1 : 0)
    kIndex = (kCycle < 161 ? 0 : 1)
    kOut seqtime2 kReset, 1, 3, 3, kIndex, giZero
    if kCycle == 1 || kCycle == 161 then
      kExpected = .5
    elseif kCycle == 65 || kCycle == 225 then
      kExpected = .25
    elseif kCycle == 97 || kCycle == 257 then
      kExpected = .75
    endif
  elseif p4 == 5 then
    ; Changing the time unit keeps the pending event's deadline.
    kUnit init 1
    kUnit = (kCycle < 33 ? 1 : 2)
    kOut seqtime2 0, kUnit, 0, 4, 0, giZero
    if kCycle == 1 || kCycle == 65 then
      kExpected = .5
    elseif kCycle == 129 then
      kExpected = 1.5
    endif
  elseif p4 == 6 then
    ; Reset changes the next index without moving its pending deadline.
    ; Reusing the trigger variable as output must also work.
    kOut = (kCycle == 33 ? 1 : 0)
    kIndex = (kCycle < 33 ? 0 : 3)
    kOut seqtime2 kOut, 1, 0, 4, kIndex, giZero
    if kCycle == 1 then
      kExpected = .5
    elseif kCycle == 65 then
      kExpected = .75
    endif
  elseif p4 == 7 then
    ; Keep MYFLT precision in the event output.
    kOut seqtime2 0, 1, 0, 2, 0, giPrecise
    if kCycle == 1 then
      kExpected table 1, giPrecise
    endif
  elseif p4 == 8 then
    ; A table change affects the next event, not its pending deadline.
    kTable init giZero
    kTable = (kCycle < 33 ? giZero : giDelay)
    kOut seqtime2 0, 1, 0, 4, 0, kTable
    if kCycle == 1 then
      kExpected = .5
    elseif kCycle == 65 then
      kExpected = .125
    elseif kCycle == 81 then
      kExpected = .5
    endif
  elseif p4 == 9 then
    ; Zero intervals still consume one table element per control cycle.
    kOut seqtime2 0, 1, 0, 4, 0, giEmptyIntervals
    if kCycle == 2 || kCycle == 51 then
      kExpected = .25
    elseif kCycle == 33 then
      kExpected = .125
    endif
  elseif p4 == 11 then
    ; Play the intro once, then wrap to the nonzero loop start.
    kOut seqtime2 0, 1, 2, 4, 0, giZero
    if kCycle == 1 then
      kExpected = .5
    elseif kCycle == 65 || kCycle == 193 then
      kExpected = .25
    elseif kCycle == 97 || kCycle == 225 then
      kExpected = .75
    endif
  else
    kOut seqtime2 0, 1, 3, 3, 0, giZero
    if kCycle == 1 then
      kExpected = .5
    elseif kCycle == 65 then
      kExpected = .25
    elseif kCycle == 97 then
      kExpected = .75
    endif
  endif
  if kOut != kExpected then
    printks "seqtime2 case %g cycle %g: got %.12g, expected %.12g\n", 0, p4, kCycle, kOut, kExpected
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 1 1
i 1 0 1.7 2
i 1 0 1.1 3
i 1 0 2.2 4
i 1 0 1.1 5
i 1 0 1 6
i 1 0 .1 7
i 1 0 1 8
i 1 0 .4 9
i 1 0 2 10
i 1 0 2 11
e
</CsScore>
</CsoundSynthesizer>
