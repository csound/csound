<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o comb.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

gamix init 0 

instr 1 

kcps    expon p5, p3, p4
asig	vco2  0.3, kcps
	outs  asig, asig 

gamix = gamix + asig 

endin

instr 99 

krvt =  3.5
ilpt =  0.1
aleft	comb gamix, krvt, ilpt
aright	comb gamix, krvt, ilpt*.2
	outs   aleft, aright

clear gamix	; clear mixer
 
endin

</CsInstruments>
<CsScore>

i 1 0 3 20 2000
i 1 5 .01 440 440

i 99 0 8
e

</CsScore>
</CsoundSynthesizer>
