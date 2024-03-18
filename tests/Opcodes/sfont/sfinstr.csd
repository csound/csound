<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfinstr.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32
nchnls = 2
0dbfs  = 1 

; By  Menno Knevel - 2020

gisf	sfload	"sf_GMbank.sf2"
	sfilist	gisf

instr 1	

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/500000						;scale amplitude
kfreq	=	1						;do not change freq from sf
a1,a2	sfinstr	ivel, inum, kamp*ivel, kfreq, 194, gisf		;= Strings 2 tighter, make amp velocity dependent
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
