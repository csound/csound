;iv

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>

<CsInstruments>
sr 		= 		48000
ksmps 		= 	   48
nchnls 		= 		2
0dbfs 		= 		1


gisine		ftgen		0, 0, 131072, 10, 1
gioctfn		ftgen		0, 0, 4096, -19, 1, 0.5, 270, 0.5
gasendL		init		0
gasendR		init		0
ginotes		ftgen		0, 0, -100, -17, 0, 8.00, 10, 8.03, 15, 8.04, 25, 8.05, 50, 8.07, 60, 8.08, 73, 8.09, 82, 8.11
gkDuration	init		15
		seed		0
		alwayson	"read_channels"		
		alwayson	"rescale_controls"		
		alwayson	"trigger_notes"
		alwayson	"reverb"

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		(4-1)/(16-1),"slider1"
gkslider2	chnget		"slider2"
		chnset		(1-0.5)/(8-0.5),"slider2"
gkslider3	chnget		"slider3"
		chnset		(1-0.25)/(4-0.25),"slider3"
gkslider4	chnget		"slider4"
		chnset		(1-0.25)/(4-0.25),"slider4"
gkslider5	chnget		"slider5"
		chnset		2/5,"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		0.5,"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		(0.9-0.5)/(1-0.5),"trackpad.y"
		endin

		instr		rescale_controls
gkDensity	scale		gkslider1,8,0.5
gkNVoices	scale		gkslider2,16,1
gkSpeed		scale		gkslider3,4,0.25
gkSpin		scale		gkslider4,4,0.25
gkSpread	scale		gkslider5,5,0
gkRvbMix	scale		gktrackpadx,1,0
gkRvbSize	scale		gktrackpady,1,0.4
		endin

		instr		trigger_notes
krate		rspline		0.04, 0.15, 0.05, 0.1
ktrig		metro		krate*gkDensity
gktrans		init		0
gktrans		trandom		ktrig,-1, 1
gktrans		=		semitone(gktrans)
idur		=		15
if ktrig==1 then
 reinit START_CLUSTER
endif
START_CLUSTER:
icount		=		1
loop:
		event_i		"i","hboscil_note",rnd(i(gkSpread)),i(gkDuration)
loop_le	icount,1,i(gkNVoices),loop
		endin

		instr		hboscil_note
ipch		table		int(rnd(100)),ginotes
icps		=		cpspch(ipch)*i(gktrans)*semitone(rnd(0.5)-0.25)
kamp		expseg		0.001,0.02,0.2,p3-0.01,0.001
ktonemoddep	jspline		0.01,0.05,0.2
ktonemodrte	jspline		6,0.1,0.2
ktone		oscil		ktonemoddep,ktonemodrte,gisine
kbrite		rspline		-2,3,0.0002*gkSpeed,3*gkSpeed
ibasfreq	init		icps
ioctcnt		init		2
iphs		init		0
a1 		hsboscil 	kamp, ktone, kbrite, ibasfreq, gisine, gioctfn, ioctcnt, iphs	
amod		oscil		1, ibasfreq*3.47, gisine
arm		=		a1*amod
kmix		expseg		0.001, 0.01, rnd(1), rnd(3)+0.3, 0.001
a1		ntrpol		a1, arm, kmix
a1 		pareq 		a1/10, 400, 15, .707
a1		tone		a1, 500
kpanrte		jspline		5, 0.05, 0.1
kpandep		jspline		0.9, 0.2, 0.4
apan		oscili		kpandep, kpanrte*gkSpin, gisine
a2		=		a1*(1-apan)
a1		=		a1*apan

a1		delay		a1, rnd(0.1)
a2		delay		a2, rnd(0.1)
kenv		linsegr		1, 1, 0
a1		=		a1*kenv
a2		=		a2*kenv
		outs		a1*(1-gkRvbMix), a2*(1-gkRvbMix)
gasendL		=		gasendL+(a1*gkRvbMix)/2.5
gasendR		=		gasendR+(a2*gkRvbMix)/2.5
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
