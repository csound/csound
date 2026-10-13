<CsTest>
description = "Delay readers stay with their instrument or UDO across nesting and reinit"
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

opcode ChildDelay, a, i
  iValue xin
  aRead delayr .01
  delayw a(iValue)
  xout aRead
endop

instr Child
  aRead delayr .01
  delayw a(.4)
  out aRead
endin

instr Nested
  ; The parent's reader is still waiting when both children initialize.
  aParent delayr .01
  aUDO ChildDelay .3
  aChild subinstr "Child"
  aTap deltap .005
  delayw a(p4)

  if timeinsts() > .03 then
    if abs(k(aParent) - p4) > .00001 || abs(k(aTap) - p4) > .00001 \
        || abs(k(aUDO) - .3) > .00001 || abs(k(aChild) - .4) > .00001 then
      printks "A nested delay paired with another instance's reader\n", 0
      exitnowk -1
    endif
  endif
endin

instr Indices
  ; Writers consume readers in order. Taps default to the newest reader.
  aFirst, iFirst delayr .01
  aSecond, iSecond delayr .01
  aThird, iThird delayr .01
  if iFirst != -1 || iSecond != -2 || iThird != -3 then
    prints "delayr returned the wrong reader indices\n"
    exitnow -1
  endif
  aNewest deltap .005
  aPrevious deltapi .005, 1
  aOldest deltapn 40, 2
  aByIndex deltap3 .005, iFirst
  aWindowed deltapx a(.005), 4, iSecond
  ; Add zero through the write tap to exercise its source lookup too.
  deltapxw a(0), a(.005), 4, iThird
  delayw a(.1)
  delayw a(.2)
  delayw a(.3)

  if timeinsts() > .03 then
    if abs(k(aFirst) - .1) > .00001 || abs(k(aSecond) - .2) > .00001 \
        || abs(k(aThird) - .3) > .00001 || abs(k(aNewest) - .3) > .00001 \
        || abs(k(aPrevious) - .2) > .00001 || abs(k(aOldest) - .1) > .00001 \
        || abs(k(aByIndex) - .1) > .00001 || abs(k(aWindowed) - .2) > .00001 then
      printks "Delay writer order or tap index changed\n", 0
      exitnowk -1
    endif
  endif
endin

instr Reinit
  kCycle init 0
  if kCycle == 80 then
    reinit LINE
  endif
LINE:
  ; iskip=1 must retain the samples as well as reconnecting the writer.
  aRead delayr .004, 1
  aTap deltapn 16
  delayw a(p4)
  rireturn
  if kCycle > 40 then
    if abs(k(aRead) - p4) > .00001 || abs(k(aTap) - p4) > .00001 then
      printks "Reinit lost the delay samples or the reader\n", 0
      exitnowk -1
    endif
  endif
  kCycle += 1
endin

instr PendingReinit
  kCycle init 0
  if kCycle == 2 then
    reinit READER
  endif
READER:
  ; Reinitializing an unmatched reader must not add it to the queue twice.
  aRead, iIndex delayr .004, 1
  if iIndex != -1 then
    prints "Reinit queued the same delay reader twice\n"
    exitnow -1
  endif
  rireturn
  kCycle += 1
endin
</CsInstruments>
<CsScore>
; Overlap parent notes, then reuse an ended instance with a different value.
i "Nested" 0 .08 .1
i "Nested" .02 .08 .2
i "Nested" .12 .08 .5
i "Indices" .22 .08
i "Reinit" .32 .04 .6
i "PendingReinit" .38 .01
e
</CsScore>
</CsoundSynthesizer>
