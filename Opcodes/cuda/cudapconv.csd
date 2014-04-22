<CsoundSynthesizer>
<CsOptions>
--opcode-lib=libcudaop5.dylib --opcode-lib=libcudaop4.dylib
</CsOptions>
<CsInstruments>
ksmps = 8
nchnls =2
i1 ftgen  1,0,2^19,1,"/Users/victor/audio/metheny.wav",0,0,1
i2 ftgen  2,0,64,1,"/Users/victor/audio/metheny.wav",0,0,1
;i1 ftgen  1,0,65536,7,1,1,1,0,65534,0

instr 1

;asig oscili 0dbfs/4, 440;
 
asig diskin2 "/Users/victor/audio/cornetto.wav",1,0,1
a1 cudapconv asig,1,64
a2 cudaconv asig,2
;a1 ftconv asig,1,64
;a1 pconvolve asig,"/Users/victor/audio/church_00.wav",1024
    out a1/2000, a1/2000
endin

</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
 