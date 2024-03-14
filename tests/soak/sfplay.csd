<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -+rtmidi=virtual -M0 ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfplay.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32
nchnls = 2
0dbfs  = 1 


isf	sfload	"07AcousticGuitar.sf2"
	sfplist	isf
	sfpassign 0, isf

instr 1	; play from score and midi keyboard

	mididefault	60, p3
	midinoteonkey	p4, p5
inum	init	p4
ivel	init	p5
ivel	init	ivel/127					;make velocity dependent
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/5000						;scale amplitude
kfreq	init	1						;do not change freq from sf
a1,a2	sfplay	ivel, inum, kamp*ivel, kfreq, 0			;preset index = 0
	outs	a1, a2
	
	endin
	
</CsInstruments>
<CsScore>
f0  60				; stay active for 1 minute

i1 0 1 60 127
i1 + 1 62 <
i1 + 1 65 <
i1 + 1 69 10

e
</CsScore>
</CsoundSynthesizer>
