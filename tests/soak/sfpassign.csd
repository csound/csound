<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac ;;;realtime audio out, virtual midi in
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfpassign.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By  Menno Knevel - 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;load three soundfonts
gisf	sfload	"sf_GMbank.sf2"
gir	sfload	"01hpschd.sf2"
giv	sfload	"07AcousticGuitar.sf2"
	sfplist gisf
	sfplist gir
        sfplist giv

; first, sf_GMbank.sf2 is loaded and assigned to start at 0 and counting up to 328
; as there are 329 presets in sf_GMbank.sf2.
; then 01hpschd.sf2 is loaded and assigned to replace the 100th preset of sf_GMbank.sf2
; then 07AcousticGuitar.sf2 is loaded and assigned to replace the 20th preset of sf_GMbank.sf2

	sfpassign	0, gisf	
	sfpassign	100, gir
        sfpassign	20, giv

instr 1	

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/600000 * ivel					;scale amplitude and velocity dependent
kfreq	=	1						;do not change freq from sf
a1,a2	sfplay3	ivel, inum, kamp, kfreq, p6			;preset index starts at 0, counting up
	outs	a1, a2
	
	endin
	
</CsInstruments>
<CsScore>

i1 0 1 60 100   0   ; Piano 1 from sf_GMbank.sf2
i1 + 1 62 <     .
i1 + 1 65 <     .
i1 + 1 69 10    .

i1 5 1 60 100   100 ; harpsichord from 01hpschd.sf2
i1 + 1 62 <     .
i1 + 1 65 <     .
i1 + 1 69 10    .

i1 10 1 60 100   20 ; guitar from 07AcousticGuitar.sf2
i1 + 1 62 <     .
i1 + 1 65 <     .
i1 + 1 69 10    .

i1 15 1 60 100   101 ; Goblin from sf_GMbank.sf2
i1 + 1 62 <     .
i1 + 1 65 <     .
i1 + 1 69 10    .
e
</CsScore>
</CsoundSynthesizer>
