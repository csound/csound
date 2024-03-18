<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     -M0  ;;;RT audio I/O with MIDI in
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cpsmidi.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; move pitch bend wheel while you play

kcps cpsmidib
asig	oscil 0.6, kcps, 1
	printk2 kcps
	outs  asig, asig

endin

</CsInstruments>
<CsScore>
f0 20
;sine wave.
f 1 0 16384 10 1

e
</CsScore>
</CsoundSynthesizer>
