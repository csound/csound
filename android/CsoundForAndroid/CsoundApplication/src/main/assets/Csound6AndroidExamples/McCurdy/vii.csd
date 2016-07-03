;vii

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>                                 

<CsInstruments>
sr 		    = 		48000
ksmps 		= 		48
nchnls 		= 		2
0dbfs 		= 		1

giampscl	ftgen		0, 0, -20000, -16, 1, 20, 0, 1, 19980, -30, 0.1
giwave		ftgen		0, 0, 4096, 9, 3, 1, 0, 10,1/10,0, 18,1/14,0, 26,1/18,0, 34,1/22,0, 42,1/26,0, 50,1/30,0, 58,1/34,0
gicos		ftgen		0, 0, 131072, 11, 1
giseq		ftgen		0, 0, -12, -2, 3/2, 2, 3, 1, 1, 3/2, 1/2, 3/4, 5/2, 2/3, 2, 1
gasendL		init		0
gasendR		init		0
		seed		0
		alwayson	"read_channels"	
		alwayson	"rescale_controls"		
		alwayson	"trigger_sequence"
		alwayson	"reverb"


		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		(1-1)/(10-1),"slider1"
gkslider2	chnget		"slider2"
		chnset		(0.5-0)/(1-0),"slider2"
gkslider3	chnget		"slider3"
		chnset		(0-(-2))/(2-(-2)),"slider3"
gkslider4	chnget		"slider4"
		chnset		(0-(-2))/(2-(-2)),"slider4"
gkslider5	chnget		"slider5"
		chnset		(1-0.1)/(2-0.1),"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		1,"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		(0.8-0.5)/(1-0.5),"trackpad.y"
		endin

		instr		rescale_controls
gkRateOfChange	scale		gkslider1,10,1
gkMix		scale		gkslider2,1,0
gkBellOctave	scale		gkslider3,2,-2
gkBellOctave	=		int(gkBellOctave)
gkStringOctave	scale		gkslider4,2,-2
gkStringOctave	=		int(gkStringOctave)
gkDuration	scale		gkslider5,2,0.1
gkRvbMix	scale		gktrackpadx,1,0
gkRvbSize	scale		gktrackpady,1,0.4
		endin


		instr		trigger_sequence
ktrig		metro		gkRateOfChange/5
		schedkwhennamed	ktrig,0,0,"trigger_notes",0,30/gkRateOfChange
iShape		ftgen		0,0,4096,-16,0,4096,-8,1
gkMix1		table		(1-gkMix)*(ftlen(iShape)-1),iShape
gkMix2		table		gkMix*(ftlen(iShape)-1),iShape
		endin

		instr		trigger_notes
itime_unit	random		2, 10
istart		random		0, 6
iloop		random		6, 13
ktrig_out 	seqtime 	int(itime_unit), int(istart), int(iloop), 0, giseq
idur		random		8, 15
inote		random		0, 48
inote		=		(int(inote))+36
kdtn		line		rnd(2), p3, -rnd(2)
kcps		=		cpsmidinn(inote+int(kdtn))
isend		random		0.1, 0.4
kflam		random		0, 0.02
kamp		rspline		0.008, 0.4, 0.05, 0.2
ioffset		random		-0.2, 0.2
kcrossfade	rspline		0, 1, 0.01, 0.1
gkcrossfade	=		kcrossfade^3
kattlim		rspline		0, 1, 0.01, 0.1
		schedkwhennamed	ktrig_out, 0, 0, "long_bell", kflam, idur*gkDuration, kcps*semitone(ioffset), isend, kamp
		event_i		"i", "gbuzz_long_note", 0, 30*i(gkDuration), cpsmidinn(inote+19)
		endin

		instr		long_bell
acps		transeg		1, p3, 3, 0.95
iattrnd		random		0, 1
iatt		=		(iattrnd>(p8^1.5)?0.002:p3/2)
aenv		expsega		0.001, iatt, 1, p3-0.2-iatt, 0.002, 0.2, 0.001
aperc		expseg		10000, 0.003, p4, 1, p4
iampscl		table		p4, giampscl
ijit		random		0.5, 1
ioctave		=		octave(i(gkBellOctave))
a1		oscili		p6*aenv*iampscl*ijit*(1-gkcrossfade)*gkMix1, (acps*aperc*ioctave                   )/2, giwave
a2		oscili		p6*aenv*iampscl*ijit*(1-gkcrossfade)*gkMix1, (acps*aperc*ioctave*semitone(rnd(.02)))/2, giwave
adlt		rspline		1, 5, 0.4, 0.8
acho		vdelay		a1, adlt, 40
a1		=		a1-acho
acho		vdelay		a2, adlt, 40
a2		=		a2-acho
icf		random		0, 1.75
icf		=		p4+(p4*(icf^3))
kcfenv		expseg		icf, 0.3, icf, p3-0.3, 20
a1		butlp		a1, kcfenv
a2		butlp		a2, kcfenv
a1		butlp		a1, kcfenv
a2		butlp		a2, kcfenv
		outs		a1, a2
gasendL		=		gasendL+(a1*p5*gkRvbMix)
gasendR		=		gasendR+(a2*p5*gkRvbMix)
		endin

		instr		gbuzz_long_note
kenv		expseg		0.001, 3, 1, p3-3, 0.001
kmul		rspline		0.01, 0.1, 0.01, 0.4
ioctave		=		octave(i(gkStringOctave))
a1		gbuzz		(kenv*gkcrossfade*gkMix2)/40, (p4*ioctave)/2, 3, 1, kmul, gicos
a1		delay		a1, rnd(0.04)+0.001
a2		delay		a1, rnd(0.04)+0.001
gasendL		=		gasendL+a1*gkRvbMix
gasendR		=		gasendR+a2*gkRvbMix
		endin

		instr		reverb
aL,aR		reverbsc	gasendL, gasendR, gkRvbSize, 10000
		outs		aL, aR
		clear		gasendL, gasendR
endin

</CsInstruments>

<CsScore>
f 0 3600
e
</CsScore>

</CsoundSynthesizer>
