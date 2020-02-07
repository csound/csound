;ii

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>

<CsInstruments>
sr 		= 		48000
ksmps 		= 	   48
nchnls 		= 		2
0dbfs 		= 		1

giampscl	ftgen		0, 0, -20000, -16, 1, 20, 0, 1, 19980, -5, 1
giwave1		ftgen		0,0,1024,9,  6,1,0,    9,1/10,0, 13,1/14,0, 17,1/18,0, 21,1/22,0,  25,1/26,0,  29,1/30,0,  33,1/34,0
giwave2		ftgen		0,0,1024,9,  7,1,0,   10,1/10,0, 14,1/14,0, 18,1/18,0, 22,1/22,0,  26,1/26,0,  30,1/30,0,  34,1/34,0
giwave3		ftgen		0,0,1024,9,  8,1,0,   11,1/10,0, 15,1/14,0, 19,1/18,0, 23,1/22,0,  27,1/26,0,  31,1/30,0,  35,1/34,0
giwave4		ftgen		0,0,1024,9,  9,1,0,   12,1/10,0, 16,1/14,0, 20,1/18,0, 24,1/22,0,  28,1/26,0,  32,1/30,0,  36,1/34,0
giwave5		ftgen		0,0,1024,9, 10,1,0,   13,1/10,0, 17,1/14,0, 21,1/18,0, 25,1/22,0,  29,1/26,0,  33,1/30,0,  37,1/34,0
giwave6		ftgen		0,0,1024,9, 11,1,0,   19,1/10,0, 28,1/14,0, 37,1/18,0, 46,1/22,0,  55,1/26,0,  64,1/30,0,  73,1/34,0
giwave7		ftgen		0,0,1024,9, 12,1/4,0, 25,1,0,    39,1/14,0, 63,1/18,0, 87,1/22,0, 111,1/26,0, 135,1/30,0, 159,1/34,0
giseq		ftgen		0,0,-12,-2,	1, 1/3, 1/3, 1/3, 1, 1/3, 1/3, 1/3, 1/2, 1/2, 1/2, 1/2
gidurs		ftgen		0,0,-100,-17,	0, 0.4, 50, 0.8, 90, 1.5
gasendL		init		0
gasendR		init		0
gamixL		init		0
gamixR		init		0

girescales	ftgen		0,0,-7,-2,6,7,8,9,10,11,12

		alwayson	"read_channels"	
		alwayson	"rescale_controls"		
		alwayson	"start_sequences"
		alwayson	"sound_output"
		alwayson	"reverb"
		seed		0

		opcode		tonea, a, ai
		setksmps	1
ain, ipeak_cf	xin
kcfenv		transeg		10000, ipeak_cf, -8, 20, 1, 0, 20
aout		tone		ain, kcfenv	
		xout		aout
		endop

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		1,"slider1"
gkslider2	chnget		"slider2"
		chnset		0.5,"slider2"
gkslider3	chnget		"slider3"
		chnset		(1-0.5)/(8-0.5),"slider3"
gkslider4	chnget		"slider4"
		chnset		0,"slider4"
gkslider5	chnget		"slider5"
		chnset		0,"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		0.5,"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		(0.75-0.5)/(1-0.5),"trackpad.y"
		endin

		instr		rescale_controls
gkSpeed		scale		gkslider1,4,1
gkSpeed		=		int(gkSpeed)
gkOctaveShift	scale		gkslider2,1,-1
gkDurations	scale		gkslider3,8,0.5
gkFlam		scale		gkslider4,0.25,0.02
gkGliss		scale		gkslider5,0.95,0
gkRvbMix	scale		gktrackpadx,1,0
gkRvbSize	scale		gktrackpady,1,0.4
		endin

		instr		start_sequences
ktrig		metro		1/4
		schedkwhennamed	ktrig, 0, 0, "play_sequence", 0, 48
		endin

		instr		play_sequence
itime_unit	random		2, 5
istart		random		0, 6
iloop		random		6, 13
ktrig_out 	seqtime 	(int(itime_unit)*4)/(3*i(gkSpeed)), int(istart), int(iloop), 0, giseq
inote		random		48, 100
ienvscl		=		((1-(inote-48)/(100-48))*0.8)+0.2
ienvscl		limit		ienvscl,0.3,1
icps		=		cpsmidinn(int(inote))
ipan		random		0, 1
isend		random		0.3, 0.5
kamp		rspline		0.007, 0.6, 0.05, 0.2
kflam		random		0, gkFlam
ifn		random		0, 7
		schedkwhennamed	ktrig_out, 0, 0, "play_note", kflam, 0.01, icps, ipan, isend, kamp, int(ifn), ienvscl
		endin

		instr		play_note
idurndx		random		0, 100
p3		table		idurndx, gidurs
p3		=		p3*i(gkDurations)
ijit		random		0.1, 1
acps		expseg		8000, 0.003, p4, 1, p4
aenv		expsega		0.001, 0.003, ijit^2, (p3-0.2-0.002)*p9, 0.002, 0.2, 0.001, 1, 0.001
adip		transeg		1, p3, 2, 0.99-i(gkGliss)
iampscl		table		p4, giampscl
irescale	table		p8, girescales
idtn		random		0.995,1.005
a1		poscil		p7*aenv*iampscl, (acps*adip*idtn*octave(gkOctaveShift))/(6+irescale), giwave1+p8
icf		random		0, 2
icf		limit		p4+(p4*icf^3), 20, 3000
a1		butlp		a1, icf
a1		tonea		a1,p9
a1, a2		pan2		a1, p5
gamixL		=		gamixL + a1
gamixR		=		gamixR + a2
gasendL		=		gasendL + (a1*(p6^2)*gkRvbMix)
gasendR		=		gasendR + (a2*(p6^2)*gkRvbMix)
		endin

		instr		sound_output
a1,a2		reverbsc	gamixL, gamixR, 0.01, 500
a1		=		a1*100
a2		=		a2*100
a1		atone		a1, 250
a2		atone		a2, 250
		outs		a1*(1-gkRvbMix), a2*(1-gkRvbMix)
		clear		gamixL, gamixR
		endin
		
		instr		reverb
aL, aR		reverbsc	gasendL, gasendR, gkRvbSize, 4000
		outs		aL, aR
		clear		gasendL, gasendR
		endin

</CsInstruments>

<CsScore>
</CsScore>

</CsoundSynthesizer>
