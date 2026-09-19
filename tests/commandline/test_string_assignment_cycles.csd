<CsTest>
description = "strcpyk copies current channel text in each local cycle"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

instr CopyChannel
  setksmps p4
  kCycle eventcycles
  Sexpected sprintfk "cycle %d", kCycle
  if kCycle % 2 == 1 then
    Sexpected strcpyk ""
  endif
  chnset Sexpected, "text"
  Sreceived chnget "text"
  Sfirst strcpyk Sreceived
  Ssecond strcpyk Sfirst
  ; A channel read and both copies must agree, including empty strings.
  if strcmpk(Sreceived, Sexpected) != 0 then
    printks "Channel read failed on local cycle %d\n", 0, kCycle
    exitnowk -1
  endif
  if strcmpk(Sfirst, Sexpected) != 0 || strcmpk(Ssecond, Sexpected) != 0 then
    printks "strcpyk is stale on local cycle %d\n", 0, kCycle
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
; Include delayed starts and smaller local blocks than the engine uses.
i "CopyChannel" .0625 .125 32
i "CopyChannel" .25 .125 4
i "CopyChannel" .5 .125 1
e
</CsScore>
</CsoundSynthesizer>
