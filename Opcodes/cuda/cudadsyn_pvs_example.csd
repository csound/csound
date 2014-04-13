<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libcudaop1.dylib --opcode-lib=./libcudaop2.dylib 
</CsOptions>
<CsInstruments>

ksmps = 64
0dbfs = 1


instr 1
ifftsize = 1024
ihopsize = 256
ibins = 512
asig1,adp diskin "/users/victor/audio/metheny.wav",1,0,1
fsig = pvsanal(asig1, ifftsize,ihopsize, ifftsize, 1)
asig = cudasynth(fsig,1,1,ibins)
asig = linenr(asig,0.001,0.01,0.01)
    out(asig*0.5)

endin


</CsInstruments>
<CsScore>
i1 0 60
</CsScore>
</CsoundSynthesizer>

