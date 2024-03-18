<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfplist.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By  Menno Knevel - 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisf	sfload	 "sf_GMbank.sf2"
	sfplist	 gisf					;list all 329 presets of Soundfont
gir	sfpreset 125, 3, gisf, 0 			;choose preset 125 = Car Pass

instr 1

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/700000					;scale amplitude
kfreq	=	1					;do not change freq from sf
a1,a2	sfplay3	ivel, inum, kamp*ivel, kfreq, gir       ; make amp velocity dependent
	outs	a1, a2
	
endin
	
</CsInstruments>
<CsScore>

i1 0 1 60 127
i1 + 1 62 <
i1 + 1 65 <
i1 + 1 69 10

e
</CsScore>
</CsoundSynthesizer>
