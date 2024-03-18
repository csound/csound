<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac ;;;realtime audio out, virtual midi in
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfload.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By  Menno Knevel - 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; load in two soundfonts
isf	sfload	"sf_GMbank.sf2"
ir	sfload	"07AcousticGuitar.sf2"
	sfplist isf
	sfplist ir
; first sf_GMbank.sf2 is loaded and assigned to start at 0 and counting up to 328
; as there are 329 presets in sf_GMbank.sf2 (0-328).
; then 07AcousticGuitar.sf2 is loaded and assigned to replace the 10th preset of already loaded sf_GMbank.sf2
	sfpassign	0, isf	
	sfpassign	10, ir

instr 1 ; play French Horn, bank 0 program 60

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/500000						;scale amplitude
kfreq	=	1						;do not change freq from sf
a1,a2	sfplay3	ivel, inum, kamp*ivel, kfreq, 60			;preset index = 60
	outs	a1, a2	
endin
	
instr 2	; play Guitar replaces sf_GMbank.sf2 at preset index 10

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/700000						;scale amplitude
kfreq	=	1						;do not change freq from sf
a1,a2	sfplay3	ivel, inum, kamp*ivel, kfreq, 10			;preset index = 10
	outs	a1, a2	
endin
	
</CsInstruments>
<CsScore>

i1 0 1 60 100
i1 + 1 62 <
i1 + 1 65 <
i1 + 1 69 10

i2 5 1 60 100
i2 + 1 62 <
i2 7 1 65 <
i2 7 1 69 10

e
</CsScore>
</CsoundSynthesizer>
