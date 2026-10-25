<CsTest>
description = "reverbsc and reverbsc2 preserve their delay lines when reinitialization is skipped"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 2
0dbfs = 1
gkFinished init 0

instr CheckTail
  iVariant = p4
  iPitchMod = p5
  aInputLeft poscil .1, 220
  aInputRight poscil .08, 330
  ; This copy runs without interruption.
  if iVariant == 1 then
    aReferenceLeft, aReferenceRight reverbsc aInputLeft, aInputRight, .85, 6000, sr, iPitchMod
  else
    aReferenceLeft, aReferenceRight reverbsc2 aInputLeft, aInputRight, .85, 6000, sr, iPitchMod
  endif

  kBlock init 0
  if kBlock == 128 || kBlock == 512 then
    reinit KEEP_TAIL
  endif
KEEP_TAIL:
  ; iskip=1 must keep every delay position, sample, filter and random state.
  if iVariant == 1 then
    aLeft, aRight reverbsc aInputLeft, aInputRight, .85, 6000, sr, iPitchMod, 1
  else
    aLeft, aRight reverbsc2 aInputLeft, aInputRight, .85, 6000, sr, iPitchMod, 1
  endif
  rireturn

  ; Compare all samples in each block, including those just after reinit.
  kErrorLeft maxk aLeft - aReferenceLeft, 1, 1
  kErrorRight maxk aRight - aReferenceRight, 1, 1
  if !(kErrorLeft == 0 && kErrorRight == 0) then
    printks "reverbsc variant=%g modulation=%g block=%g error=%g/%g\n", 0, iVariant, iPitchMod, kBlock, kErrorLeft, kErrorRight
    exitnowk -1
  endif
  kSignal maxk aReferenceLeft, 1, 1
  if kBlock == 700 then
    if !(kSignal > .001) then
      printks "The reference reverb must produce a signal\n", 0
      exitnowk -1
    endif
    gkFinished += 1
  endif
  kBlock += 1
endin

instr CheckCompletion
  if i(gkFinished) != 6 then
    prints "Not all reverb cases completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Both opcodes, with modulation disabled, at its default, and at its maximum.
i "CheckTail" 0 1 1 0
i "CheckTail" 0 1 1 1
i "CheckTail" 0 1 1 20
i "CheckTail" 0 1 2 0
i "CheckTail" 0 1 2 1
i "CheckTail" 0 1 2 20
i "CheckCompletion" 1 .01
e
</CsScore>
</CsoundSynthesizer>
