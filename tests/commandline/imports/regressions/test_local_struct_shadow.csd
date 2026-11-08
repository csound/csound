<CsTest>
expect.exit=0
expect.output=["LOCAL_STRUCT_SHADOW_PASS"]
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
struct LocalVec x:i,y:i
vec@global:LocalVec init 10,20
instr 1
 vec:LocalVec init 100,200
 assert vec.x==100 && vec.y==200
 vec.x=300
 assert vec.x==300
endin
instr 2
 assert vec.x==10 && vec.y==20
 prints "LOCAL_STRUCT_SHADOW_PASS\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 .02 .01
e
</CsScore>
</CsoundSynthesizer>
