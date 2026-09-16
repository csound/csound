<CsTest>
description = "seed preserves a nonzero 31-bit state and accepts unsigned 32-bit seeds"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
gkChecks init 0

instr 1
  ; Score fields are MYFLT: use seeds exactly representable at that precision.
  iWide = (16777217 > 16777216 ? 1 : 0)
  iSeed = (iWide == 1 ? p4 : p6)
  iExpected = (iWide == 1 ? p5 : p7)
  seed iSeed
  iState getseed
  if iState != iExpected then
    prints "seed %g: expected state %g, got %g\n", iSeed, iExpected, iState
    exitnow -1
  endif
  iFirst unirand 1
  seed iSeed
  iAgain unirand 1
  if iFirst != iAgain then
    prints "seed %g: repeated seed changed the random sequence\n", iSeed
    exitnow -1
  endif
  if iSeed != iExpected then
    seed iExpected
    iReduced unirand 1
    if iFirst == iReduced then
      prints "seed %g: lost high bits of the Mersenne Twister seed\n", iSeed
      exitnow -1
    endif
  endif
  seed iSeed
  kDust dust 1, kr
  kDust2 dust2 1, kr
  kPrevious init -1
  kCycle timeinstk
  if kDust <= 0 || kDust >= 1 || kDust2 <= -1 || kDust2 >= 1 || kDust == kPrevious then
    printks "seed %g: random generator stalled, dust=%g dust2=%g\n", 0, iSeed, kDust, kDust2
    exitnowk -1
  endif
  kPrevious = kDust
  if kCycle == 3 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 7 then
    prints "seed checks did not complete\n"
    exitnow -1
  endif
  seed 0
  iState getseed
  if iState < 1 || iState >= 2147483647 then
    prints "clock seed produced an invalid generator state\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Double seed/state pairs, followed by float seed/state pairs.
; Clock-seed unit tests cover the exact integer boundaries in both builds.
i1 0 .01 1 1 1 1
i1 .02 .01 1234 1234 1234 1234
i1 .04 .01 2147483646 1 16777216 16777216
i1 .06 .01 2147483647 1 2147483520 2147483520
i1 .08 .01 2147483648 2 2147483648 2
i1 .1 .01 4294967292 1 2147483904 258
i1 .12 .01 4294967295 3 2147484160 514
i99 .14 .001
</CsScore>
</CsoundSynthesizer>
