<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  aW = 1
  aZero = 0
  aU = 1
  aInput[] init p4
  aInput[0] = aW
  aArray[] init 5

  if p4 == 4 then
    aL, aR, aC, aSL, aSR bformdec1 3, aW, aZero, aZero, aZero
  elseif p4 == 9 then
    aInput[7] = aU
    aL, aR, aC, aSL, aSR bformdec1 3, aW, aZero, aZero, aZero, \
                                  aZero, aZero, aZero, aU, aZero
  else
    aInput[7] = aU
    aL, aR, aC, aSL, aSR bformdec1 3, aW, aZero, aZero, aZero, \
                                  aZero, aZero, aZero, aU, aZero, \
                                  aZero, aZero, aZero, aZero, aZero, aZero, aZero
  endif
  aArray bformdec1 3, aInput

  iHigherOrder = p4 > 4 ? 1 : 0
  aFront = aW * (.405 + .085 * iHigherOrder)
  aCentre = aW * (.085 + .045 * iHigherOrder)
  aSurround = aW * (.635 - .08 * iHigherOrder)
  aError = abs(aL - aFront) + abs(aR - aFront) + abs(aC - aCentre)
  aError += abs(aSL - aSurround) + abs(aSR - aSurround)
  aError += abs(aArray[0] - aFront) + abs(aArray[1] - aFront)
  aError += abs(aArray[2] - aCentre)
  aError += abs(aArray[3] - aSurround) + abs(aArray[4] - aSurround)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "bformdec1 with %d inputs lost or changed active samples: %g\n", 0, p4, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 9 then
    prints "bformdec1 checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Whole blocks, then a partial start and end, then a partial end alone.
i 1 0 .004 4
i 1 0 .004 9
i 1 0 .004 16
i 1 .010625 .003 4
i 1 .010625 .003 9
i 1 .010625 .003 16
i 1 .02 .0035 4
i 1 .02 .0035 9
i 1 .02 .0035 16
i 99 .03 .002
</CsScore>
</CsoundSynthesizer>
