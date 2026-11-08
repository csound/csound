<CsTest>
expect.exit=0
description="Module bindings preserve identity, types, aliases, overloads and instrument scope"
expect.output=["MODULE_BINDINGS_PASS"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --run-unit-tests
</CsOptions>
<CsInstruments>
sr=44100
ksmps=32
nchnls=2
0dbfs=1
import "a.orc" as first
import "./a.orc" as same
import "b.orc" as second
from "a.orc" import giValue as iFirst
from "a.orc" import giValue as iAgain
from "a.orc" import GetValue as Renamed, Voice as ImportedVoice
from "a.orc" import giValues, gSLabel
instr 1
 iA=first.GetValue(0)
 iB=second.GetValue(0)
 iRenamed=Renamed(1)
 iSingle first.Over 3
 iDouble=first.Over(3,4)
 assert iA==11 && iB==22 && iRenamed==12
 assert iSingle==4 && iDouble==7
 assert iFirst==11 && iAgain==11
 assert giValues[1]==2 && first.giValues[2]==3
 assert strcmp(gSLabel,"alpha")==0
 assert first.gVec.x==10 && first.gVec.y==20
 first.gVec.x=30
 assert same.gVec.x==30
 first.giValue=99
 assert same.giValue==99 && iFirst==99 && iAgain==99
 assert second.giValue==22
 schedule ImportedVoice,0,.001
 schedule second.Voice,0,.001
endin
instr 2
 iA chnget "voice_a"
 iB chnget "voice_b"
 assert iA==99 && iB==22
 prints "MODULE_BINDINGS_PASS\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 .02 .01
e
</CsScore>
</CsoundSynthesizer>
