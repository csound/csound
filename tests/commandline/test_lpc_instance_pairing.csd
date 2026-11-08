<CsTest>
description = "LPC consumers keep their own sources and selected slot through nested init and reinit"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

; Each portable LPANAL text file holds one frame with two coefficients.
; With a constant .1 input, feedback .5 settles at .2 and .75 at .4.
opcode ChildFilter, a, 0
  lpslot 99
  kR, kO, kE, kC lpread 0, "lpc_slots_three_quarters.lpc"
  aOut lpreson a(.1)
  xout aOut
endop

instr Child
  lpslot 7
  kR, kO, kE, kC lpread 0, "lpc_slots_three_quarters.lpc"
  aOut lpreson a(.1)
  out aOut
endin

instr Parent
  lpslot p4
  kR, kO, kE, kC lpread 0, "lpc_slots_half.lpc"
  ; Both children select another slot before the parent's filters initialize.
  aUDO ChildFilter
  aChild subinstr "Child"
  kCycle init 0
  if kCycle == 100 then
    reinit FILTERS
  endif
FILTERS:
  ; A filter-only reinit must retain the parent's source and selected slot.
  aNormal lpreson a(.1)
  aShifted lpfreson a(.1), 1
  rireturn
  if kCycle == 80 || kCycle == 180 then
    if abs(k(aNormal) - .2) > .00001 || abs(k(aShifted) - .2) > .00001 \
        || abs(k(aUDO) - .4) > .00001 || abs(k(aChild) - .4) > .00001 then
      printks "LPC filters paired with another instance's source\n", 0
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kCycle += 1
endin

instr ReusedDefault
  ; The first note leaves slot 70 selected. The next must start at slot 0.
  if p4 == 1 then
    lpslot 70
  endif
  kR, kO, kE, kC lpread 0, "lpc_slots_half.lpc"
  if p4 == 0 then
    lpslot 0
  endif
  aOut lpreson a(.1)
  if timeinstk() == 80 then
    if abs(k(aOut) - .2) > .00001 then
      printks "A reused note retained its previous LPC slot selection\n", 0
      exitnowk -1
    endif
    gkChecks += 1
  endif
endin

instr CheckCompletion
  if i(gkChecks) != 8 then
    prints "Not every LPC filter check completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Overlapping notes use the same slot number with independent sources.
i "Parent" 0 .03 0
i "Parent" .005 .03 0
; Exercise a larger slot number on a reused instance.
i "Parent" .05 .03 70
i "ReusedDefault" .1 .02 1
i "ReusedDefault" .13 .02 0
i "CheckCompletion" .17 0
e
</CsScore>
</CsoundSynthesizer>
