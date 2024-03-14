<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      -M0  ;;;realtime audio out and midi in
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKDrummer.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;STK Drummer - has no controllers but plays samples (11)

icps	cpsmidi
iamp	ampmidi	1
asig	STKDrummer icps, iamp
	outs asig, asig
endin

</CsInstruments>
<CsScore>
; play 5 minutes
f0 300
e
</CsScore>
</CsoundSynthesizer>
