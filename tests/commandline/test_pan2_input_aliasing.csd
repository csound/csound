<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 2
0dbfs = 1
gkchecks init 0

instr 1
  aSource oscili .5, 440
  kPan = .25
  aRefL, aRefR pan2 aSource, kPan, p4

  aLeftCopy = aSource
  aLeftCopy, aOtherR pan2 aLeftCopy, kPan, p4
  aRightCopy = aSource
  aOtherL, aRightCopy pan2 aRightCopy, kPan, p4
  aStereo[] pan2 aSource, kPan, p4

  aError = abs(aRefL - aLeftCopy) + abs(aRefR - aOtherR)
  aError += abs(aRefL - aOtherL) + abs(aRefR - aRightCopy)
  aError += abs(aRefL - aStereo[0]) + abs(aRefR - aStereo[1])
  kerror max_k aError, 1, 1
  if !(kerror <= .000001) then
    printks "pan2 mode %d differs with input reuse (k-rate pan): %g\n", 0, p4, kerror
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 2
  aSource oscili .5, 440
  aPan phasor 250
  aRefL, aRefR pan2 aSource, aPan, p4

  aLeftCopy = aSource
  aLeftCopy, aOtherR pan2 aLeftCopy, aPan, p4
  aRightCopy = aSource
  aOtherL, aRightCopy pan2 aRightCopy, aPan, p4
  aStereo[] pan2 aSource, aPan, p4

  aError = abs(aRefL - aLeftCopy) + abs(aRefR - aOtherR)
  aError += abs(aRefL - aOtherL) + abs(aRefR - aRightCopy)
  aError += abs(aRefL - aStereo[0]) + abs(aRefR - aStereo[1])
  kerror max_k aError, 1, 1
  if !(kerror <= .000001) then
    printks "pan2 mode %d differs with input reuse (a-rate pan): %g\n", 0, p4, kerror
    exitnowk(-1)
  endif
  kfirst init 1
  if kfirst == 1 then
    gkchecks += 1
    kfirst = 0
  endif
endin

instr 99
  if i(gkchecks) != 16 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .004 0
i 1 0 .004 1
i 1 0 .004 2
i 1 0 .004 3
i 2 0 .004 0
i 2 0 .004 1
i 2 0 .004 2
i 2 0 .004 3
i 1 .010375 .001 0
i 1 .010375 .001 1
i 1 .010375 .001 2
i 1 .010375 .001 3
i 2 .010375 .001 0
i 2 .010375 .001 1
i 2 .010375 .001 2
i 2 .010375 .001 3
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
