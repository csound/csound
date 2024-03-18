<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   No messages  MIDI in
-odac           -d         -M0  ;;;RT audio out with MIDI in
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

kaft aftouch 0, 1
printk2 kaft

;aftertouch from music keyboard used for volume control
asig	oscil 0.7 * kaft, 220, 1
	outs  asig, asig

endin

</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0 30
e

</CsScore>
</CsoundSynthesizer>
