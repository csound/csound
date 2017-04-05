<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac     ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o gen30.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

;a simplified example of Istvan Varga
sr =  44100
ksmps =  32
nchnls =  2
0dbfs   = 1

isaw	ftgen 1, 0, 16384, 7, 1, 16384, -1						;sawtooth wave 
iFM	ftgen 3, 0, 4096, 7, 0, 512, 0.25, 512, 1, 512, 0.25, 512,	\
			     0, 512, -0.25, 512, -1, 512, -0.25, 512, 0			;FM waveform
iAM	ftgen 4, 0, 4096, 5, 1, 4096, 0.01						;AM waveform
iEQ	ftgen 5, 0, 1024, 5, 1, 512, 32, 512, 1						;FM to EQ
isine	ftgen 6, 0, 1024, 10, 1								;sine wave

/* generate bandlimited sawtooth waves */
i0	=  0
loop1:
imaxh	=  sr / (2 * 440.0 * exp(log(2.0) * (i0 - 69) / 12))
i_	ftgen i0 + 10, 0, 4096, -30, 1, 1, imaxh					;use gen 30
i0	=  i0 + 1
	if (i0 < 127.5) igoto loop1

instr 1

kcps	=  440.0 * exp(log(2.0) * (p4 - 69) / 12)					;note frequency
klpmaxf	limit p5 * kcps, 1000.0, 12000.0						;lowpass max. frequency

kfmd1	=  0.03 * kcps									;FM depth in Hz
kamfr	=  kcps * 0.02									;AM frequency
kamfr2	=  kcps * 0.1

kfnum	=  (10 + 69 + 0.5 + 12 * log(kcps / 440.0) / log(2.0))				;table number
aenv	linseg 0, p3*0.25, 1, p3*0.75, 0						;amp. envelope

asig	oscbnk kcps, 0.0, kfmd1, 0.0, 40, 200, 0.1, 0.2, 0, 0, 144,	      \
		0.0, klpmaxf, 0.0, 0.0, 1.5, 1.5, 2, kfnum, 3, 0, 5, 5, 5
asig	= asig * aenv*.03
outs asig, asig

endin

</CsInstruments>
<CsScore>
s
i 1 0 6 41 10
i 1 0 6 60
i 1 0 6 65
i 1 0 6 69
s
i 1 0 6 41 64
i 1 0 6 60
i 1 0 6 65
i 1 0 6 69

e
</CsScore>
</CsoundSynthesizer>

