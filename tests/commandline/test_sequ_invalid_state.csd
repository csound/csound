<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef ID
#define ID #0#
#endif
sr = 8192
ksmps = 16
nchnls = 1
instr 1
 kCount, kMap[] sequstate $ID
endin
</CsInstruments>
<CsScore>
i 1 0 .001
e
</CsScore>
</CsoundSynthesizer>
