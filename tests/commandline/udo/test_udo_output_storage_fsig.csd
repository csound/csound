<CsTest>
description = "UDO spectral output storage survives rebinding and instance reuse"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=48000
ksmps=32
nchnls=1
gkSource init 8
opcode Mixed(source:f):(f,k)
 result:f = pvsgain(source, 1)
 xout result, gkSource
endop
opcode Repeated(source:f):(f,f)
 result:f = pvsgain(source, 1)
 xout result, result
endop
instr 1
 fSource pvsinit 1024
 fMixed, kGlobal = Mixed(fSource)
 objects:Opcode[] = create(opcoderef("Repeated"), 1)
 fInit1, fInit2 init objects[0], fSource
 fOut1 pvsinit 1024
 fOut2 pvsinit 1024
 fOut1, fOut2 perf objects[0], fSource
 aMixed pvsynth fMixed
 aFirst pvsynth fOut1
 aSecond pvsynth fOut2
 if kGlobal != 8 then
  exitnowk -1
 endif
 delete(objects)
endin
</CsInstruments>
<CsScore>
i1 0 .02
i1 .03 .02
</CsScore>
</CsoundSynthesizer>
