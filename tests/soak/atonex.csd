<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o atonex.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; unfiltered noise

asig	rand	0.7	; white noise
	outs	asig, asig

endin

instr 2	; filtered noise

asig	rand	0.7
khp	line	100, p3, 3000
afilt	atonex	asig, khp, 32

; Clip the filtered signal's amplitude to 85 dB.
a1	clip afilt, 2, ampdb(85)
	outs a1, a1
endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 2 2

e

</CsScore>
</CsoundSynthesizer>