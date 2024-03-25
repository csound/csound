<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac       ;   -iadc    ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o Mixer.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps =	32
nchnls = 2
0dbfs  = 1

instr 1

katt	expon 0.01, p3, 1		;create an attack
aout	poscil .7, 440,1
	MixerSetLevel	1, 3, katt	;impose attack on the gain level
	MixerSend aout, 1, 3, 0		;send to channel 0
endin

instr 2

aout	vco2 .5, 110			;saw wave
	MixerSetLevel 2, 3, .25		;set level to .25 of vco2
	MixerSend aout, 2, 3, 1		;send to channel 1
endin

instr 3	;mix instr.1 and 2 with reverb
	
kgain1	MixerGetLevel	1,3		;get level form buss 3
kgain2	MixerGetLevel	2,3		;get level form buss 3
a1	MixerReceive	3,0		;receive channel 0
a2	MixerReceive	3,1		;receive channel 1
aout	= a1*kgain1+a2*kgain2		;mix them
aoutL, aoutR reverbsc aout, aout,  0.85, 12000	;add a nice reverb
	outs  aoutL, aoutR
	MixerClear
endin

</CsInstruments>
<CsScore>
f1 0 4096 10 1

i1 0 2
i2 0 2
i3 0 8	;reverb stays on for 8 sec.

e
</CsScore>
</CsoundSynthesizer>

