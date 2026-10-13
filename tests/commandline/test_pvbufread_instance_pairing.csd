<CsTest>
description = "pvinterp and pvcross retain their instance's reader across nested init and reinit"
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
gkChecks init 0

instr MakeFiles
  aTone oscili .2, 500
  fSmall pvsanal aTone, 128, 32, 128, 1
  fLarge pvsanal aTone, 256, 64, 256, 1
  pvsfwrite fSmall, "pvoc.77"
  pvsfwrite fLarge, "pvoc.78"
endin

opcode ChildReader, 0, 0
  pvbufread .05, 78
endop

instr Child
  pvbufread .05, "pvoc.78"
  out a(0)
endin

instr Parent
  ; The children use a different FFT size. Borrowing their reader would
  ; make the parent's consumers fail with a frame-size mismatch.
  pvbufread .05, "pvoc.77"
  ChildReader
  aChild subinstr "Child"

  kCycle init 0
  if kCycle == 40 then
    reinit CONSUMERS
  endif
CONSUMERS:
  ; Reinitializing only the consumers must still find this parent's reader.
  aInterp pvinterp .05, 1, "pvoc.77", 1, 1, 1, 1, .5, .5
  aCross pvcross .05, 1, 77, 1, 0
  rireturn
  kInterp rms aInterp
  kCross rms aCross
  if kCycle == 30 || kCycle == 70 then
    if !(kInterp > .0001) || !(kCross > .0001) then
      printks "The parent's spectral reader did not reach both consumers\n", 0
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kCycle += 1
endin

instr CheckCompletion
  if i(gkChecks) != 6 then
    prints "Not every parent reached both spectral checks\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Close both output files before any reader opens them.
i "MakeFiles" 0 .1
i "Parent" .15 .2
i "Parent" .19 .2
; Reuse an ended parent and its nested instances.
i "Parent" .45 .2
i "CheckCompletion" .7 0
e
</CsScore>
</CsoundSynthesizer>
