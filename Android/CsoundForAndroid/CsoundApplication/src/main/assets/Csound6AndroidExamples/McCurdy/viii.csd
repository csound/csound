;viii

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>

<CsInstruments>
sr          =       48000
ksmps 		=  		48
nchnls 		= 		2
0dbfs 		= 		1

gidurs		ftgen		0, 0, -100, -17, 0,4, 5,1, 45,1/2, 70,1/4, 90,1/3
gilens		ftgen		0, 0, -100, -17, 0,0.4, 85,4
giwave		ftgen		0, 0, 4096, 10, 1, 0, 0, 0, 0.05
gisine		ftgen		0, 0, 4096, 10, 1
gasendL		init		0
gasendR		init		0
		seed		0

		alwayson	"read_channels"		
		alwayson	"rescale_controls"		
		alwayson	"start_3_sequences"
		alwayson	"reverb"

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		(1-0.2)/(5-0.2),"slider1"
gkslider2	chnget		"slider2"
		chnset		(0-(-2))/(2-(-2)),"slider2"
gkslider3	chnget		"slider3"
		chnset		(12-0)/(12-0),"slider3"
gkslider4	chnget		"slider4"
		chnset		(500-1)/(500-1),"slider4"
gkslider5	chnget		"slider5"
		chnset		(0.3-0)/(1-0),"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		(0.4-0.4)/(4-0.4),"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		(4-0.4)/(4-0.4),"trackpad.y"
		endin

		instr		rescale_controls
gkDensity	scale		gkslider1,5,0.2
gkOctave	scale		gkslider2,2,-2
gkbase		scale		gkslider3,12,0
gkrange		scale		gkslider4,500,1
gkRvbMix	scale		gkslider5,1,0
gkDurMin	scale		gktrackpadx,4,0.4
gkDurMax	scale		gktrackpady,4,0.4
		endin

		instr		start_3_sequences
		event_i		"i", "sequence", 0, 60*60*24*7
		event_i		"i", "sequence", 0, 60*60*24*7
		event_i		"i", "sequence", 0, 60*60*24*7
		turnoff
		endin

		instr		sequence
kndx		randomh		0,1, 1
krate		table		kndx, gidurs, 1
ktrig		metro		(2*gkDensity)/krate
knote		randomh		0, 12, 0.1
kamp		rspline		0, 0.1, 1, 2
kpan		rspline		0.1, 0.9, 0.1, 1
kmul		rspline		0.1, 0.9, 0.1, 0.3
		schedkwhen	ktrig, 0, 0, "note", rnd(0.1), 0.01, int(knote)*3, kamp, kpan, kmul
		endin

		instr		note
iratio		=		int(rnd(20))+1
p3		table		rnd(1), gilens, 1
p3		limit		p3,i(gkDurMin),i(gkDurMax)
aenv		expseg		1, p3, 0.001
aperc		expseg		5, 0.001, 1, 1, 1
iprob		random		0, 1
		if iprob<=0.1 then
irange		random		-8, 4
icurve		random		-4, 4
abend		transeg		1, p3, 0, semitone(irange)
aperc		=		aperc*abend
		endif
kmul		expon		abs(p7), p3, 0.0001


a1 		gbuzz 		p5*aenv, cpsmidinn(p4)*iratio*aperc*octave(i(gkOctave)), int(rnd(i(gkrange))), rnd(i(gkbase))+1, kmul, giwave
iprob2		random		0,1
		if iprob2<=0.2&&p3>1 then
kfshift 	transeg 	0, p3, -15, rnd(200)-100
ar,ai		hilbert  	a1
asin		oscili		1, kfshift, gisine, 0
acos		oscili   	1, kfshift, gisine, 0.25
amod1		=		ar*acos
amod2		= 		ai*asin
a1		= 		((amod1-amod2)/3)+a1
		endif
a1		butlp		a1, cpsoct(rnd(8)+4)
a1,a2		pan2		a1, p6
a1		delay		a1, rnd(0.03)+0.001
a2		delay		a2, rnd(0.03)+0.001
		outs		a1, a2
gasendL		=		gasendL+a1*gkRvbMix
gasendR		=		gasendR+a2*gkRvbMix
		endin

		instr		reverb
aL,aR		reverbsc	gasendL, gasendR, 0.75, 10000
		outs		aL, aR
		clear		gasendL, gasendR
		endin

</CsInstruments>

<CsScore>
f 0 3600
e
</CsScore>

</CsoundSynthesizer>
