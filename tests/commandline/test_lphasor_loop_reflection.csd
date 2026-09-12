<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iOffset = int(p2*sr + .5) % ksmps
  kCount init 0
  kBlock init 0
  kPhase init p7
  kDirection init 1
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  aStep init 0
  kN = 0
  while kN < ksmps do
    kStep = p5
    if p9 != 0 then
      kStep += .5*((kCount + kN - kStart + 3) % 3)
    endif
    vaset kStep, kN, aStep
    kN += 1
  od
  if kBlock == 2 && p8 != 0 then
    reinit PLAY
  endif
PLAY:
  iSkip = (i(kBlock) == 0 ? 0 : p8)
  if p6 == 0 then
    aActual lphasor p5, 0, 4, p4, p7, iSkip
  elseif p6 == 1 then
    aActual lphasor aStep, 0, 4, p4, p7, iSkip
  else
    aStep lphasor aStep, 0, 4, p4, p7, iSkip
    aActual = aStep
  endif
  rireturn
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kExpected = kPhase
      kStep = p5
      if p9 != 0 then
        kStep += .5*((kCount + kN - kStart) % 3)
      endif
      kPhase += kDirection*kStep
      if p4 != 0 then
        ; Follow each boundary crossing separately as a reference.
        while (kDirection == 1 && kPhase >= 4) || (kDirection == -1 && kPhase <= 0) do
          if kDirection == 1 then
            if p4 == 2 || p4 == 3 then
              kPhase = 8 - kPhase
              kDirection = -1
            else
              kPhase -= 4
            endif
          else
            if p4 == 1 || p4 == 3 then
              kPhase = -kPhase
              kDirection = 1
            else
              kPhase += 4
            endif
          endif
        od
      endif
    endif
    kActual vaget kN, aActual
    if kActual != kExpected then
      printks "FAIL lphasor mode=%g step=%g rate=%g sample=%g: %g expected %g\n", \
          0, p4, p5, p6, kCount + kN - kStart, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd - kStart
  kBlock += 1
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 20 then
    prints "FAIL lphasor checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; mode, step, input/output reuse, initial phase, skip reinit, varying step.
i 1 0.0 .0625 3 6 0 0 0 0
i 1 0.1279296875 .0625 3 6 1 0 0 0
i 1 0.2529296875 .0625 3 6 2 0 0 0
i 1 0.375 .0625 3 10 1 0 0 1
i 1 0.5029296875 .0625 3 18 0 0 0 0
i 1 0.6279296875 .0625 3 8 1 0 0 0
i 1 0.75 .0625 3 4 0 0 0 0
i 1 0.8779296875 .0625 3 0 0 2 0 0
i 1 1.0029296875 .0625 3 0.5 1 -2 0 0
i 1 1.125 .0625 1 6 0 0 0 0
i 1 1.2529296875 .0625 1 8 1 0 0 0
i 1 1.3779296875 .0625 1 18 2 0 0 0
i 1 1.5 .0625 2 6 0 0 0 0
i 1 1.6279296875 .0625 2 8 1 0 0 0
i 1 1.7529296875 .0625 2 18 2 0 0 0
i 1 1.875 .0625 0 6 0 0 0 0
i 1 2.0029296875 .0625 0 0.5 2 -2 0 1
i 1 2.1279296875 .0625 3 6 0 0 1 0
i 1 2.25 .0625 3 6 1 0 -1 0
i 1 2.3779296875 .0625 2 1 1 0 2 0
i 99 2.625 .015625
</CsScore>
</CsoundSynthesizer>
