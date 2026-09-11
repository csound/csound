<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef WINDOW
#define WINDOW #0#
#endif
#ifndef AUDIO
#define AUDIO #1#
#endif
sr = 8192
ksmps = 16
nchnls = 1
instr 1
 if $AUDIO == 1 then
  aInput = 1
  aResult median aInput, $WINDOW, 5
 else
  kResult mediank 1, $WINDOW, 5
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
