<CsoundSynthesizer>
<CsOptions>
--opcode-lib=libcudaop4.dylib
</CsOptions>
<CsInstruments>
ksmps = 128
i1 ftgen  1,0,1024,1,"/Users/victor/audio/church.wav",0,0,1

instr 1

asig diskin2 "/Users/victor/audio/cornetto.wav",1,01
a1 cudaconv asig,1
;a1 dconv asig,ftlen(1),1
    out a1/20
endin

</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
 
