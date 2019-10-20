<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  
aout diskin2 "beats.wav", 1, 0, 1
kfco line 100, p3, 10000		;filter-cutoff
krez init p4
asig moogvcf2 aout, kfco, krez
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 4 .1
i 1 + 4 .6
i 1 + 4 .9
e
</CsScore>
</CsoundSynthesizer>
