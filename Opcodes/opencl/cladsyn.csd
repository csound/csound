<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libclopp.dylib
</CsOptions>
<CsInstruments>

ksmps = 1
0dbfs = 1


instr 1
ifftsize = 2048
ihopsize = 512
ibins = 1024
idev =  1; /* device number */
asig1 vco2 0.5, 300
fsig = pvsanal(asig1, ifftsize,ihopsize, ifftsize, 1)
;asig = cladsyn(fsig,1,1,ibins,idev)
;asig = linenr(asig,0.001,0.01,0.01)
 ;  out(asig*0.5)

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
giSize  =      8192
giFreq  ftgen   0, 0, giSize, 7, 0, giSize, 0
giAmps  ftgen   0, 0, giSize, 7, 0, giSize, 0


instr 3
iup = 512
iam ftgen 0,0,1024,7,1,1024,0.001
ifr ftgen 0,0,1024,-5,100,1024,5000

idev =  1; /* device number */
;asig = cladsyn(0.01,1,iam,ifr,iup,idev)

;asig adsynt 1/100, 1, -1, ifr, iam, 1024
asig = linenr(asig,0.001,0.01,0.01)
    out(asig*0.5)

endin

</CsInstruments>
<CsScore>
i3 0 60
i3 0 60
i3 0 60
i3 0 60
</CsScore>
</CsoundSynthesizer>

