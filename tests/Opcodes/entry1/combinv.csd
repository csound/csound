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

instr 1 

kcps    expon p5, p3, p4
asig	oscil3 0.3, kcps
krvt =  3.5
ilpt =  0.1
aleft	combinv asig, krvt, ilpt
	outs   aleft, asig

endin

</CsInstruments>
<CsScore>
i 1 0 3 20 2000
e

</CsScore>
</CsoundSynthesizer>
