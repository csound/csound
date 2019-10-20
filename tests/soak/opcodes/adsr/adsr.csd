<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

iatt  = p5
idec  = p6  
islev = p7
irel  = p8

kenv	adsr iatt, idec, islev, irel
kcps =  cpspch(p4) 	  ;frequency

asig	vco2  kenv * 0.8, kcps
	outs  asig, asig

endin

</CsInstruments>
<CsScore>

i 1  0  1  7.00  .0001  1  .01  .001 ; short attack
i 1  2  1  7.02  1  .5  .01  .001    ; long attack
i 1  4  2  6.09  .0001  1 .1  .7     ; long release

e

</CsScore>
</CsoundSynthesizer>
