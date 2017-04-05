<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o xadsr.wav -W ;;; for file output any platform
</CsOptions>
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

kenv	xadsr iatt, idec, islev, irel
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
