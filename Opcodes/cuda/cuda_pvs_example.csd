<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libcudaop2.dylib
</CsOptions>
<CsInstruments>

ksmps = 64
0dbfs = 1

instr 1
ifftsize = 2048
ihopsize = 512
asig = diskin2:a("flutec3.wav",1,0,1)
fsig = cudanal(asig, 
               ifftsize, 
               ihopsize, 
               ifftsize, 1)
asig = cudasynth(fsig)
asig = linenr(asig,0.005,0.01,0.01)    
   out(asig)
endin

instr 2
S1 = "flutec3.wav"
ifftsize = 2048
ihopsize = 512
asig  diskin2 S1, 1, 0, 1
fsig pvsanal asig, ifftsize, ihopsize, ifftsize, 1
a1 pvsynth fsig
a2 linenr a1*0.5,0.005,0.01,0.01    
   out a2
endin


</CsInstruments>
<CsScore>
i2 0 60
</CsScore>
</CsoundSynthesizer>

