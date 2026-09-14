<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
giCounter cntCreate 3
instr 1
 kValue count p4
endin
instr 2
 kValue cntRead p4
endin
instr 3
 kValue cntCycles p4
endin
instr 4
 cntReset p4
endin
instr 5
 kMax, kMin, kInc cntState p4
endin
instr 6
 iValue count_i p4
endin
instr 7
 iHandle cntcreate 3
 iDeleted cntdeletei iHandle
 iValue counti iHandle
endin
</CsInstruments>
<CsScore>
; Only handle zero exists; one is unused and ten is the capacity boundary.
i 1 0 .01 1
i 1 0 .01 10
i 1 0 .01 -1
i 1 0 .01 1e30
i 2 0 .01 1
i 3 0 .01 1
i 4 0 .01 1
i 5 0 .01 1
i 6 0 .01 1
i 7 .02 .01
e
</CsScore>
</CsoundSynthesizer>
