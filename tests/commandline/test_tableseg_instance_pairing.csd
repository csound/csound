<CsTest>
description = "vpvoc keeps its own instance's table envelope across nested calls and reinit"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
giFull ftgen 1, 0, 64, -7, 1, 64, 1
giHalf ftgen 2, 0, 64, -7, .5, 64, .5
giSilent ftgen 3, 0, 64, -2, 0
gkChecks init 0

instr MakeFile
  aTone oscili .2, 500
  fAnalysis pvsanal aTone, 128, 32, 128, 1
  pvsfwrite fAnalysis, "pvoc.81"
endin

opcode ChildEnvelope, 0, 0
  tableseg giSilent, 1, giSilent
endop

instr Child
  tablexseg giSilent, 1, giSilent
  out a(0)
endin

instr Parent
  ; Each spelling must publish an envelope owned by this parent.
  if p5 == 0 then
    tableseg p4, 1, p4
  elseif p5 == 1 then
    tablexseg p4, 1, p4
  else
    ktableseg p4, 1, p4
  endif
  ; These silent envelopes must not replace the parent's envelope.
  ChildEnvelope
  aChild subinstr "Child"

  kCycle init 0
  if kCycle == 40 then
    reinit READERS
  endif
READERS:
  ; Both readers restart together. One uses the local tableseg, the other
  ; uses the same envelope as an explicit table number.
  aActual vpvoc .05, 1, "pvoc.81"
  aExpected vpvoc .05, 1, 81, 0, p4
  rireturn
  kError max_k abs(aActual - aExpected), 1, 1
  if !(kError < .00001) then
    printks "vpvoc used another instance's envelope\n", 0
    exitnowk -1
  endif
  kLevel rms aActual
  if kCycle == 30 || kCycle == 70 then
    if !(kLevel > .0001) then
      printks "The parent's nonzero envelope produced silence\n", 0
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kCycle += 1
endin

instr TwoEnvelopes
  ; A later envelope in the same instance must not redirect an earlier reader.
  tableseg giFull, 1, giFull
  aFull vpvoc .05, 1, "pvoc.81"
  tablexseg giHalf, 1, giHalf
  aHalf vpvoc .05, 1, "pvoc.81"
  aExpected vpvoc .05, 1, "pvoc.81", 0, giFull
  kError max_k abs(aFull - aExpected) + abs(2*aHalf - aExpected), 1, 1
  if !(kError < .00001) then
    printks "A later tableseg redirected an earlier vpvoc\n", 0
    exitnowk -1
  endif
  kCycle init 0
  if kCycle == 30 then
    gkChecks += 1
  endif
  kCycle += 1
endin

instr CheckCompletion
  if i(gkChecks) != 7 then
    prints "Not every envelope check ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "MakeFile" 0 .1
; Overlap parents, then reuse an ended parent with the old alias.
i "Parent" .15 .2 1 0
i "Parent" .19 .2 2 1
i "Parent" .45 .2 1 2
i "TwoEnvelopes" .7 .1
i "CheckCompletion" .85 0
e
</CsScore>
</CsoundSynthesizer>
