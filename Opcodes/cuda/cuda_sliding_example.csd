<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libcudaop3.dylib
</CsOptions>
<CsInstruments>

ksmps = 64
0dbfs = 1

instr 1
asig = diskin:a("flutec3.wav",1,0,1)
amod = p4; oscil:a(2,3)
asig2 = cudasliding(asig,amod,p5)
asig = linenr(asig2,0.005,0.01,0.01)    
   out(asig*0.5)
endin

</CsInstruments>
<CsScore>
i1 0 10 1 1024
;i1 0 5 0.5 1024
</CsScore>
</CsoundSynthesizer>

