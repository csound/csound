<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o bqrez.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1			;sawtooth waveform.

kfco  line 200, p3, 2000;filter-cutoff frequency from .2 to 5 KHz.
kres  = p4		;resonance
imode = p5		;mode
asig	vco 0.2, 220, 1
afilt	bqrez asig, kfco, kres, imode
asig	balance afilt, asig
	outs asig, asig

endin

</CsInstruments>
<CsScore>
;sine wave
f 1 0 16384 10 1

i 1 0 3 1 0		; low pass
i 1 + 3 30 0		; low pass
i 1 + 3 1 1		; high pass
i 1 + 3 30 1		; high pass

e

</CsScore>
</CsoundSynthesizer>
