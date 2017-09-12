<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o alpass.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

gamix init 0 

instr 1 

kcps    expon p4, p3, p5
asig	vco2  0.6, kcps
	outs  asig, asig 

gamix = gamix + asig 

endin

instr 99 

krvt =  3.5
ilpt =  0.1
aleft	alpass gamix, krvt*1.5, ilpt
aright	alpass gamix, krvt, ilpt*2
	outs   aleft, aright

gamix = 0	; clear mixer
 
endin

</CsInstruments>
<CsScore>

i 1 0 3 20 2000

i 99 0 8
e

</CsScore>
</CsoundSynthesizer>
