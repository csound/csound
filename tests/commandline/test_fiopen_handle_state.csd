<CsTest>
description = "fiopen returns distinct usable handles and releases shared and reinitialized files"

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

giFirst fiopen "fiopen_first.txt", 0
giSecond fiopen "fiopen_second.txt", 2
giSame fiopen "fiopen_first.txt", 0
fprints giFirst, "alpha"
fprints giSame, "beta"
fprints giSecond, "second"
giSameBinary fiopen "fiopen_second.txt", 2
fprints giSameBinary, " again"
giHeaderDone init 1
giChecks init 0
giSlot init -1
giPersistent init -1
giReinits init 0
gkChecks init 0

instr 1
  if giHeaderDone != 1 || giFirst == giSecond || giSame != giFirst || giSameBinary != giSecond then
    prints "fiopen did not return the expected handles\n"
    exitnow -1
  endif
  SFirst, iLine readfi "fiopen_first.txt"
  SSecond, iLine readfi "fiopen_second.txt"
  if strcmp(SFirst, "alphabeta") != 0 || strcmp(SSecond, "second again") != 0 then
    prints "fiopen wrote to the wrong file or truncated an open file\n"
    exitnow -1
  endif
  giChecks += 1
endin

instr 2
  iFile fiopen "fiopen_shared.txt", 0
  iSame fiopen "fiopen_shared.txt", 0
  if iFile == giFirst || iFile == giSecond || iSame != iFile then
    exitnow -1
  endif
  giSlot = iFile
  ; Closing must wait for both users to finish.
  fprintks iFile, "held"
  fprintks iSame, " twice"
  ficlose iFile
  giChecks += 1
endin

instr 3
  iFile fiopen "fiopen_replacement.txt", 0
  if iFile != giSlot then
    prints "fiopen did not release file slot %g, got %g\n", giSlot, iFile
    exitnow -1
  endif
  SShared, iLine readfi "fiopen_shared.txt"
  if strcmp(SShared, "held twice") != 0 then
    prints "ficlose closed a file while it was still in use\n"
    exitnow -1
  endif
  ficlose iFile
  giChecks += 1
endin

instr 4
  iFile fiopen "fiopen_reinit.txt", 0
  ; One borrower keeps the handle valid while the other reinitializes.
  fprintks iFile, "stay"
  kCycle timeinstk
  if kCycle == 3 then
    reinit reopen
  endif
reopen:
  giReinits += 1
  if iFile != giSlot then
    exitnow -1
  endif
  fprintks iFile, "reinit"
  rireturn
  if kCycle == 5 then
    gkChecks += 1
    turnoff
  endif
endin

instr 5
  ; Exercise the numeric signature with a string p-field and both read modes.
  iFile fiopen p4, p5
  if iFile != giSlot then
    exitnow -1
  endif
  ficlose iFile
  giChecks += 1
endin

instr 99
  if giChecks != 9 || giReinits != 4 || i(gkChecks) != 2 then
    prints "fiopen checks did not finish: %g %g %g\n", giChecks, giReinits, i(gkChecks)
    exitnow -1
  endif
endin

instr 6
  kCycle timeinstk
  if kCycle == 3 then
    reinit reopen
  endif
reopen:
  giReinits += 1
  iFile fiopen "fiopen_persistent.txt", 0
  giPersistent = iFile
  fprints iFile, "append"
  rireturn
  if kCycle == 5 then
    gkChecks += 1
    turnoff
  endif
endin

instr 7
  ; The handle must survive the note that opened it.
  fprints giPersistent, "later"
  ficlose giPersistent
  SText, iLine readfi "fiopen_persistent.txt"
  if strcmp(SText, "appendappendlater") != 0 then
    prints "fiopen reinitialization truncated the file\n"
    exitnow -1
  endif
  giChecks += 1
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 .01 .001
i3 .02 .001
i2 .03 .001
i3 .04 .001
i4 .05 .01
i3 .07 .001
i5 .08 .001 "fiopen_shared.txt" 1
i5 .09 .001 "fiopen_shared.txt" 3
i6 .11 .01
i7 .13 .001
i99 .15 .001
</CsScore>
</CsoundSynthesizer>
