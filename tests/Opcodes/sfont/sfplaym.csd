<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sfplaym.wav -W ;;; for file output any platform

; By  Menno Knevel - 2020

</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32
nchnls = 2
0dbfs  = 1 


gisf	sfload	"07AcousticGuitar.sf2"
	sfplist	gisf
	sfpassign 100, gisf         ;in this case, the number 100 is assigned to the preset

instr 1	

inum	=	p4
ivel	=	p5
kamp	linsegr	1, 1, 1, .1, 0
kamp  = kamp * .000001		        ;scale amplitude- small value due to 0dbfs = 0
a1	sfplaym	ivel, inum, kamp * ivel, p6, 100, 1  ; flag = 1 so frequencies is used intead of midi pitch
	outs	a1, a1
	
endin
	
</CsInstruments>
<CsScore>

i1 0 1 60 127 100  ; use sample of this key and set new frequency
i1 + 1 62 <    <
i1 + 1 65 <    <
i1 + 1 69 10   70

e
</CsScore>
</CsoundSynthesizer>
