<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef MODE
#define MODE #3#
#endif
#ifndef DELAY1
#define DELAY1 #16#
#endif
#ifndef DELAY2
#define DELAY2 #6#
#endif
#ifndef DELAY3
#define DELAY3 #10#
#endif
sr = 8192
ksmps = 16
nchnls = 1
instr 1
 aInput = 1
 aResult nestedap aInput, $MODE, 1, $DELAY1/sr, .2, $DELAY2/sr, .3, $DELAY3/sr, .4
endin
</CsInstruments>
<CsScore>
i 1 0 .001
e
</CsScore>
</CsoundSynthesizer>
