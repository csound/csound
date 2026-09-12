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
 aInput = 0
 aTwo harmon2 aInput, 9, 1, 1, 0, p4
endin
instr 2
 aInput = 0
 aThree harmon3 aInput, 9, 1, 1, 1, 0, p4
endin
instr 3
 aInput = 0
 aFour harmon4 aInput, 9, 1, 1, 1, 1, 0, p4
endin
; Reject unsupported controls after one valid performance cycle.
instr 4
 kCycle init 0
 kOct = (kCycle == 0 ? 9 : p4)
 kVoice = (kCycle == 0 ? 512 : p5)
 aInput = 0
 aTwo harmon2 aInput, kOct, kVoice, 0, 1, 6
 kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01 20
i 1 0 .01 -100
i 1 0 .01 1e30
i 1 0 .01 -1e30
i 2 0 .01 20
i 3 0 .01 20
i 4 0 .02 20 512
i 4 0 .02 1e30 512
i 4 0 .02 9 1e30
i 4 0 .02 9 -1e30
e
</CsScore>
</CsoundSynthesizer>
