<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cpsoct.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1 ; Convert octave-point-decimal value into Hz

ioct =	p4
icps =	cpsoct(ioct)
	print icps
asig	oscil 0.7, icps, 1
	outs  asig, asig
endin


</CsInstruments>
<CsScore>
;sine wave.
f 1 0 16384 10 1

i 1 0 1 8.75
i 1 + 1 8.77
i 1 + 1 8.79
i 1 + .5 6.30

e
</CsScore>
</CsoundSynthesizer>
