<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

imaxshake = p4
ifreq     = p5
ifreq1    = p6
ifreq2    = p7

;low amplitude
adrp dripwater .1, 0.09, 10, .9, imaxshake, ifreq, ifreq1, ifreq2 
asig clip adrp, 2, 0.9	; avoid drips that drip too loud
     outs asig, asig

endin
</CsInstruments>
<CsScore>

{100 CNT 

i1 [0.1 * $CNT] 0.5 0.5 430 1000 800 

} 

e
</CsScore>
</CsoundSynthesizer>
