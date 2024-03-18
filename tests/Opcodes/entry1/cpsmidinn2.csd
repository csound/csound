<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
;;;RT audio out, midi in, note=p4 and velocity=p5
-odac -+rtmidi=virtual -M0d --midi-key=4 --midi-velocity-amp=5
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cpsmidinn.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

massign 0, 1	;assign all midi to instr. 1

instr 1	;play virtual keyboard

inote = p4
icps  = cpsmidinn(inote)
asig	oscil 0.6, icps, 1
	print icps
	outs  asig, asig

endin

</CsInstruments>
<CsScore>
f0 20
;sine wave.
f 1 0 16384 10 1
;play note from score too
i1 0 1 60
e
</CsScore>
</CsoundSynthesizer>
