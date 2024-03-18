<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfpreset.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By  Menno Knevel - 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisf1	sfload	 "sf_GMbank.sf2"
	sfplist	 gisf1						;list presets of first soundfont
gisf2	sfload	 "07AcousticGuitar.sf2"
	sfplist	 gisf2						;list presets of second soundfont
gir	sfpreset 51, 0, gisf1, 0 				;assign Synth Strings2 (#51)to index 0
giv	sfpreset 0, 0, gisf2, 1					;assign AcousticGuitar (#0) to index 1
print gir
print giv

instr 1

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/600000						;scale amplitude, small value due to 0dbfs = 1
kfreq	=	1						;do not change freq 
a1,a2	sfplay3	ivel, inum, kamp*ivel, kfreq, p6
	outs	a1, a2
endin
	
</CsInstruments>
<CsScore>

i1 0 1 60 127 0	;= Synth Strings I from first soundfont
i1 + 1 62 <   .
i1 + 1 65 <   .
i1 + 1 69 10  .

i1 5 1 60 127 1	;= AcousticGuitar from second soundfont
i1 + 1 62 <   .
i1 + 1 65 <   .
i1 + 1 69 10  .
e
</CsScore>
</CsoundSynthesizer>
