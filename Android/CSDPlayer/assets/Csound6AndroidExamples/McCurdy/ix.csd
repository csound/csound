;ix

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>

<CsInstruments>
sr 		    = 		48000
ksmps 		= 		48
nchnls 		= 		2
0dbfs       =       1


gasendL		init		0
gasendR		init		0 
giwave		ftgen		0, 0, 128, 10, 1, 1/4, 1/16, 1/64
giampscl1	ftgen		0, 0, -20000, -16, 1, 20, 0, 1, 19980, -20, 0.01
		seed		0

		alwayson	"read_channels"		
		alwayson	"rescale_controls"		
		alwayson 	"trigger_arpeggio"
		alwayson 	"reverb"

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		(14-4)/(30-4),"slider1"
gkslider2	chnget		"slider2"
		chnset		(0-(-12))/(24-(-12)),"slider2"
gkslider3	chnget		"slider3"
		chnset		(24-(-12))/(24-(-12)),"slider3"
gkslider4	chnget		"slider4"
		chnset		(1-0.01)/(4-0.01),"slider4"
gkslider5	chnget		"slider5"
		chnset		(4-1)/(6-1),"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		1,"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		(0.88-0.5)/(1-0.5),"trackpad.y"
		endin

		instr		rescale_controls
gkRange		scale		gkslider1,30,4
gkPitchMin	scale		gkslider2,24,-12
gkPitchMax	scale		gkslider3,24,-12
gkRateOfChange	scale		gkslider4,4,0.01
gkDuration	scale		gkslider5,6,1
gkRvbMix	scale		gktrackpadx,1,0
gkRvbSize	scale		gktrackpady,1,0.99
		endin

		instr		trigger_arpeggio
krate		randomh		0.2, 0.02, 0.04
ktrig		metro		krate
		schedkwhennamed	ktrig, 0, 0, "arpeggio", 0, 25
		endin

		instr		arpeggio
krate		rspline		3, 0.1, 0.3, 0.7
ktrig		metro		krate*gkRateOfChange
kharm1		rspline		1, gkRange, 0.4, 0.8
kharm2		random		-3, 3
kharm		mirror		kharm1+kharm2, 1, 23
ibas		random		i(gkPitchMin), i(gkPitchMax)
kamp		rspline		0, 0.05, 0.1, 0.2
		schedkwhen	ktrig, 0, 0, p1+1, 0, gkDuration, cpsmidinn((int(ibas)*3)+24)*int(kharm), kamp
		endin

		instr		note_generator
aenv		linsegr		0, p3/2, 1, p3/2, 0, p3/2, 0  
iampscl		table		p4, giampscl1
asig		oscili		p5*aenv*iampscl, p4, giwave
adlt		rspline		0.01, 0.1, 0.2, 0.3
adelsig		vdelay		asig, adlt*1000, 0.1*1000
aL,aR		pan2		asig+adelsig, rnd(1)
		outs		aL, aR
gasendL		=		gasendL+aL*gkRvbMix
gasendR		=		gasendR+aR*gkRvbMix
		endin

		instr		reverb
aL, aR		reverbsc	gasendL, gasendR, gkRvbSize, 10000
		outs		aL, aR
		clear		gasendL, gasendR
endin

</CsInstruments>

<CsScore>
f 0 3600
e
</CsScore>

</CsoundSynthesizer>
