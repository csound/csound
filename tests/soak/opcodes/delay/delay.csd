<CsoundSynthesizer>
<CsInstruments>

sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

instr    1

adel	init 0
ilev    = p4				;level of direct sound
idelay  = p5 *.001			;Delay in ms
ifd	= p6				;feedback

ain	diskin2 "fox.wav", 1, 1
adel	delay   ain + (adel*ifd), idelay;ifd = amount of feedback
asig	moogvcf	adel, 1500, .6, 1	;color feedback
	outs    asig*ilev, ain

endin

</CsInstruments>
<CsScore>
;Delay is in ms
i 1  0  15 2  200 .95	;with feedback
i 1  4  5  2  20  .95
i 1  +  3  2  5   .95
i 1  +  3  3  5   0	;no feedback

e
</CsScore>
</CsoundSynthesizer>
