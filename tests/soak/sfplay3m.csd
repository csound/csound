<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfplay3m.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; By  Menno Knevel - 2020

sr = 44100 
ksmps = 32
nchnls = 2
0dbfs  = 1 


gisf	sfload	"07AcousticGuitar.sf2"
	sfplist	gisf
	sfpassign 0, gisf    ; assign the preset to number 00 

instr 1	

inum    =   p4
ivel    =   p5
kamp	linsegr	1, 1, 1, .1, 0
kamp	= kamp/900000 * ivel			;scale amplitude and make velocity dependent
kfreq	=	1						;do not change freq from sf
aout	sfplay3m ivel, inum, kamp, p6, 0, 1	   ;preset index = 0, set flag to frequency instead of midi pitch
	outs	aout, aout
	
endin
	
</CsInstruments>
<CsScore>

i1 0 1 60 127 400   ; take the samples from key 60 and set new frequency
i1 + 1 60 90   <
i1 + 1 60 60   <
i1 + 1 60 10  200

e
</CsScore>
</CsoundSynthesizer>
