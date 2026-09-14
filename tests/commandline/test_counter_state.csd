<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
gkChecks init 0

; Creating, deleting, and reusing slots must preserve live counter bindings.
instr 1
 iMissing cntDelete_i 0
 if iMissing != -1 then
  exitnow(-1)
 endif
 iHandles[] init 31
 iIndex = 0
 while iIndex < 31 do
  iHandles[iIndex] cntCreate 3, 0, 1
  if iHandles[iIndex] != iIndex then
   exitnow(-1)
  endif
  iIndex += 1
 od
 iUnused cntDelete_i 31
 iEdge cntdeletei 40
 iNegative cntDelete_i -1
 iHuge cntdeletei 1e30
 if iUnused != -1 || iEdge != -1 || iNegative != -1 || iHuge != -1 then
  exitnow(-1)
 endif
 iEven = 0
 while iEven < 31 do
  iHandle = iHandles[iEven]
  iDeleted cntDelete_i iHandle
  iAgain cntdeletei iHandle
  if iDeleted != iHandle || iAgain != -1 then
   exitnow(-1)
  endif
  iEven += 2
 od
 iEven = 0
 while iEven < 31 do
  iNew cntcreate 3, 0, 1
  if iNew != iEven then
   exitnow(-1)
  endif
  iEven += 2
 od
 iLive = iHandles[1]
 kCycle init 0
 kCycle += 1
 kValue count iLive
 kRead cntRead iLive
 kCycles cntcycles iLive
 kMax, kMin, kInc cntState iLive
 kExpected = (kCycle-1) % 4
 if kValue != kExpected || kRead != kValue+1 || kCycles != floor((kCycle-1)/4) || kMax != 3 || kMin != 0 || kInc != 1 then
  printks "counter sequence changed at cycle %g\n", 0, kCycle
  exitnowk(-1)
 endif
 if kCycle == 12 then
  gkChecks += 1
 endif
endin

; Check aliases, init-time counting, and the reset opcode's no-output layout.
instr 2
 iHandle cntcreate 4, -2, .5
 iFirst count_i iHandle
 iSecond counti iHandle
 if iFirst != 0 || iSecond != .5 then
  exitnow(-1)
 endif
 kCycle init 0
 kCycle += 1
 if kCycle % 2 == 0 then
  cntReset iHandle
 else
  cntreset iHandle
 endif
 kValue count iHandle
 kRead cntread iHandle
 kCycles cntCycles iHandle
 kMax, kMin, kInc cntstate iHandle
 if kValue != -2 || kRead != -1.5 || kCycles != 0 || kMax != 4 || kMin != -2 || kInc != .5 then
  printks "counter reset or alias mismatch\n", 0
  exitnowk(-1)
 endif
 if kCycle == 12 then
  gkChecks += 1
 endif
endin

; Growing the slot array must preserve a reader already running on a live counter.
instr 3
 iHandle cntCreate 3, 0, 1
 kCycle init 0
 kCycle += 1
 if kCycle == 4 then
  reinit GROW
 endif
 kValue count iHandle
 if kValue != (kCycle-1) % 4 then
  printks "counter changed while growing the slot array\n", 0
  exitnowk(-1)
 endif
 if kCycle == 12 then
  gkChecks += 1
 endif
 goto DONE
GROW:
 iIndex = 0
 while iIndex < 16 do
  iUnused cntCreate 3
  iIndex += 1
 od
 rireturn
DONE:
endin

instr 99
 if i(gkChecks) != 3 then
  prints "counter checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .09375
i 2 0 .09375
i 3 0 .09375
i 99 .1 .01
e
</CsScore>
</CsoundSynthesizer>
