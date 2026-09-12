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
 kPitch pitchac aInput, 256, 4096, p4
endin
; Reject an invalid minimum frequency after a valid control cycle.
instr 2
 kCycle init 0
 kMin = (kCycle == 0 ? 256 : p4)
 aInput = 0
 kPitch pitchac aInput, kMin, 4096, 256
 kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 -1
i 1 0 .01 16384
i 1 0 .01 1e-30
i 2 0 .02 0
i 2 0 .02 -1
i 2 0 .02 16384
i 2 0 .02 1e30
e
</CsScore>
</CsoundSynthesizer>
