<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfinstr3m.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32
nchnls = 2
0dbfs  = 1 

; By  Menno Knevel - 2020

gisf	sfload	"07AcousticGuitar.sf2"
	sfilist	gisf

instr 1	

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/900000						;scale amplitude, small value because of 0dbfs  = 1 
kfreq	=	1						;do not change freq from sf
aout	sfinstr3m ivel, inum, kamp*ivel, kfreq, 0, gisf
	outs	aout, aout
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
