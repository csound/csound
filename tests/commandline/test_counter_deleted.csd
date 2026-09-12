<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
 iHandle cntCreate 3
 kCycle init 0
 kCycle += 1
 if kCycle == 2 then
  kDeleted cntDelete iHandle
  if p5 != 0 then
   reinit REPLACE
  endif
 endif
 ; Each reader must report deletion, even if creation has reused the handle.
 if p4 == 0 then
  kValue count iHandle
 elseif p4 == 1 then
  kValue cntRead iHandle
 elseif p4 == 2 then
  kValue cntCycles iHandle
 elseif p4 == 3 then
  cntReset iHandle
 elseif p4 == 4 then
  kMax, kMin, kInc cntState iHandle
 elseif p4 == 5 then
  kValue cntread iHandle
 elseif p4 == 6 then
  kValue cntcycles iHandle
 elseif p4 == 7 then
  cntreset iHandle
 else
  kMax, kMin, kInc cntstate iHandle
 endif
 goto DONE
REPLACE:
 iReplacement cntcreate 9
 if iReplacement != iHandle then
  prints "counter test did not reuse the deleted slot\n"
  exitnow(-1)
 endif
 rireturn
DONE:
endin
</CsInstruments>
<CsScore>
i 1 0 .03125 0 0
i 1 0 .03125 1 0
i 1 0 .03125 2 0
i 1 0 .03125 3 0
i 1 0 .03125 4 0
i 1 0 .03125 5 0
i 1 0 .03125 6 0
i 1 0 .03125 7 0
i 1 0 .03125 8 0
; Repeat with a replacement counter created before the reader runs again.
i 1 .0625 .03125 0 1
i 1 .0625 .03125 1 1
i 1 .0625 .03125 2 1
i 1 .0625 .03125 3 1
i 1 .0625 .03125 4 1
i 1 .0625 .03125 5 1
i 1 .0625 .03125 6 1
i 1 .0625 .03125 7 1
i 1 .0625 .03125 8 1
e
</CsScore>
</CsoundSynthesizer>
