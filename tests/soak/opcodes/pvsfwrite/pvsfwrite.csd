<CsoundSynthesizer>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

;By Victor Lazzarini 2008

instr 1
asig oscili 10000, 440, 1
fss pvsanal  asig, 1024,256,1024,0
  pvsfwrite fss, "mypvs.pvx"
  ase pvsynth fss
  out ase
endin

instr 2 ; must be called after instr 1 finishes
ktim timeinsts
  fss  pvsfread ktim, "mypvs.pvx"
  asig pvsynth fss
  out asig
endin

;; added for soak tests, rm the file
instr 3
  i_ system_i 1, "rm -f mypvs.pvx"
endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1
i1 0   1
i2 1   1
i3 +   0
e
</CsScore>
</CsoundSynthesizer>
