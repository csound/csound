<CsTest>
description = "fog preserves its position input when reusing it as output"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
giPow ftgen 1,0,1024,10,1
giOther ftgen 2,0,-1023,10,1
giEnv ftgen 3,0,16,-7,1,16,1

instr 1
  aPosition = .25
  aReference fog .5,1000,1,aPosition,0,0,0,.001,0,4,p4,giEnv,.02
  aPosition fog .5,1000,1,aPosition,0,0,0,.001,0,4,p4,giEnv,.02
  kError max_k abs(aPosition-aReference),1,1
  if kError > .000001 then
    printks "fog position reuse changed the output: %g\n",0,kError
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 1
i 1 .02 .008 2
e
</CsScore>
</CsoundSynthesizer>
