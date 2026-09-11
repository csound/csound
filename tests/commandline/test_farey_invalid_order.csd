<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef ORDER
#define ORDER #0#
#endif
#ifndef KIND
#define KIND #0#
#endif
sr = 8192
ksmps = 16
nchnls = 1
instr 1
 if $KIND == 0 then
  iLength fareyleni $ORDER
 elseif $KIND == 1 then
  kLength fareylen $ORDER
 else
  iTable ftgen 0, 0, 8, "farey", $ORDER, 0
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001
e
</CsScore>
</CsoundSynthesizer>
