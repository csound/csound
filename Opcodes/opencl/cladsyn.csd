<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libclop1.dylib
</CsOptions>
<CsInstruments>

ksmps = 64
0dbfs = 1


instr 1
ifftsize = 2048
ihopsize = 512
ibins = 1024
idev =  0 ; /* device number */
asig1 vco2 0.5, 440
fsig = pvsanal(asig1, ifftsize,ihopsize, ifftsize, 1)
asig = cladsynth(fsig,1,1,ibins,idev)
asig = linenr(asig,0.001,0.01,0.01)
    out(asig*0.5)

endin

instr 2
ifftsize = 2048
ihopsize = 512
ibins = 1024
asig1 vco2 0.5, 440
fsig = pvsanal(asig1, ifftsize,ihopsize, ifftsize, 1)
asig = pvsadsyn(fsig,ibins,1)
asig = linenr(asig,0.001,0.01,0.01)
    out(asig*0.5)

endin

</CsInstruments>
<CsScore>
i1 0 60
</CsScore>
</CsoundSynthesizer>

