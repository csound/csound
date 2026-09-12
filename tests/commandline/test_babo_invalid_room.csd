<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef SIZE
#define SIZE #0#
#endif
sr = 48000
ksmps = 32
nchnls = 2
0dbfs = 1
instr 1
 aInput = 0
 aLeft, aRight babo aInput, 0, 0, 0, $SIZE, 5, 4
endin
instr 2
 aInput = 0
 aLeft, aRight babo2 aInput, 0, 0, 0, $SIZE, 5, 4
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
e
</CsScore>
</CsoundSynthesizer>
