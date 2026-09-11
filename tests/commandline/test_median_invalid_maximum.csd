<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef MAXIMUM
#define MAXIMUM #0#
#endif
sr = 8192
ksmps = 16
nchnls = 1
instr 1
 kResult mediank 1, 1, $MAXIMUM
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
