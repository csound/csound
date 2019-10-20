<CsoundSynthesizer>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
; analyze "fox.wav" with PVANAL first
ispec = p4
ktime line 0, p3, 1.55
kfrq  line .8, p3, 2
asig  pvoc ktime, kfrq, "fox.pvx", ispec 
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 6 0
i 1 + 6 1	;preserve spectral envelope
e
</CsScore>
</CsoundSynthesizer>
