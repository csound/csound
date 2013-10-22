<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o a.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; hear the difference between instr.1 and 2
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	;sine wave at k-rate

ksig	oscil 0.8, 440, 1
; k-rate to the audio-rate conversion
asig =  a(ksig)
	outs asig, asig

endin

instr 2	;sine wave at a-rate

asig	oscil 0.8, 440, 1
	outs  asig, asig

endin

</CsInstruments>
<CsScore>
; sine wave.
f 1 0 16384 10 1

i 1 0 2
i 2 2 2
e

</CsScore>
</CsoundSynthesizer>
