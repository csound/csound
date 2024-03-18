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
ksmps = 100
nchnls = 2
0dbfs = 1

gamix init 0 

instr 1 

acps    expon p4, p3, p5
asig	vco  0.6, acps, 1
	outs  asig, asig 

gamix = gamix + asig 

endin

instr 99 

arvt1 line 3.5*1.5, p3, 6
arvt2 line 3.5, p3, 4
ilpt =  0.1
aleft	alpass gamix, arvt1, ilpt
aright	alpass gamix, arvt2, ilpt*2
	outs   aleft, aright

gamix = 0	; clear mixer
 
endin

</CsInstruments>
<CsScore>
f1 0 4096 10 1
i 1 0 3 20 2000

i 99 0 8
e

</CsScore>
</CsoundSynthesizer>
