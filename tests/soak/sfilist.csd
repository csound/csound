<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfilist.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By  Menno Knevel - 2020

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisf	sfload	"sf_GMbank.sf2"
	sfilist	gisf						;lists all instruments of Soundfont

instr 1	

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/250000						;scale amplitude, small value due to 0dbfs = 1
kfreq	=	1						;do not change freq from sf
a1, a2	sfinstr3 ivel, inum, kamp*ivel, kfreq, p6, gisf        ;p6 chooses instrument, make amp velocity dependent
	outs	a1, a2
	
endin
	
</CsInstruments>
<CsScore>

i1 0 1 60 127 100       ;Halo Pad
i1 + 1 62 <     .
i1 + 1 65 <     .
i1 + 1 69 10    .

i1 5 1 60 127 1         ;Piano 2
i1 + 1 62 <     .
i1 + 1 65 <     .
i1 + 1 69 10    .

e
</CsScore>
</CsoundSynthesizer>
