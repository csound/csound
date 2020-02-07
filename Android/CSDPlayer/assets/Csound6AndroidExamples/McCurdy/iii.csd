;iii

<CsoundSynthesizer>

<CsOptions>
-odac -dm3                                          
</CsOptions>                                  

<CsInstruments>
                                                
sr 		= 		48000
ksmps 		= 	   48
nchnls 		= 		2             
0dbfs		=		1


gisine		ftgen		0,0,4096,10,1
gasendL,gasendR,gamixL,gamixR		init		0	
seed		0
gitims		ftgen		0, 0, 128, -7, 1, 100, 0.1

		alwayson	"read_channels"	
		alwayson	"rescale_controls"		
		alwayson	"start_sequences"
		alwayson	"spatialise"
		alwayson	"reverb"

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		0,"slider1"
gkslider2	chnget		"slider2"
		chnset		(4-1)/(8-1),"slider2"
gkslider3	chnget		"slider3"
		chnset		(1-0.5)/(16-0.5),"slider3"
gkslider4	chnget		"slider4"
		chnset		0,"slider4"
gkslider5	chnget		"slider5"
		chnset		(0.245-0.1)/(0.25-0.1),"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		0.5,"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		(0.6-0.5)/(1-0.5),"trackpad.y"
		endin

		instr		rescale_controls
gkNVoices	scale		gkslider1,8,1
gkDensity	scale		gkslider2,8,1
gkSpeed		scale		gkslider3,16,0.5
gkDamping	scale		gkslider4,octcps(800),octcps(20000)
gkSustain	scale		gkslider5,0.25,0.1
gkRvbMix	scale		gktrackpadx,1,0
gkRvbSize	scale		gktrackpady,1,0.4
		endin
	
		instr		start_sequences
kchanged	changed		gkNVoices
if kchanged==1 then
 reinit UPDATE
endif
UPDATE:
instances	active		p1
if instances<i(gkNVoices) then
 event_i	"i",p1,0,36000
endif
rireturn
if gkNVoices<instances then
 turnoff
endif
krate		rspline		0.075*gkDensity, 1.5*gkDensity, 0.1*gkSpeed, 0.8*gkSpeed
ktrig		metro		krate
koct		rspline		4.3, 9.5, 0.1, 1
kcps		=		cpsoct(koct)
kpan		rspline		0, 1, 0.1*gkSpeed, 1*gkSpeed
kamp		rspline		0.1, 1, 0.25*gkSpeed, 2*gkSpeed
kwgoct1		rspline		6, 9, 0.05*gkSpeed, 1*gkSpeed
kwgoct2		rspline		6, 9, 0.05*gkSpeed, 1*gkSpeed
		schedkwhennamed	ktrig, 0, 0, "wguide2_note", 0, 4, kcps, kwgoct1, kwgoct2, kamp, kpan, krate
		endin

		instr		wguide2_note
itime		times
aptr		line		0, 1/p4, 1
awavlet		tablei		aptr, gisine, 1
awavlet		=		awavlet*(p7^3)
ioct1		random		5, 11
ioct2		random		5, 11
aplk1		transeg		1+rnd(0.2), 0.1, -15, 1
aplk2		transeg		1+rnd(0.2), 0.1, -15, 1
idmptim		random		0.1, 3
kcutoff		expseg		cpsoct(i(gkDamping)), p3-idmptim, cpsoct(i(20000)), idmptim, 200, 1, 200
awg2		wguide2 	awavlet, cpsoct(p5)*aplk1, cpsoct(p6)*aplk2, kcutoff, kcutoff, gkSustain, gkSustain
awg2		dcblock2	awg2
arel		linseg		1, p3-idmptim, 1, idmptim, 0
awg2		=		awg2*arel
ktim		timeinsts
		if ktim>0.01 then
krms		rms	awg2
		if krms<0.001 then
		turnoff
		endif
		endif
awg2		=		awg2/(rnd(4)+3)
gasendL		=		gasendL+awg2/10
gasendR		=		gasendR+awg2/10
aL,aR		pan2		awg2, p8
gamixL		=		gamixL+aL
gamixR		=		gamixR+aR
		endin

		instr		spatialise
adlytim1	rspline		1, 20, 0.1, 0.4
adlytim2	rspline		1, 20, 0.1, 0.4
aL		vdelay		gamixL, adlytim1+50, 100
aR		vdelay		gamixR, adlytim2+50, 100
		outs		aL*(1-gkRvbMix), aR*(1-gkRvbMix)
gasendL		=		gasendL+(aL*gkRvbMix)/5
gasendR		=		gasendR+(aR*gkRvbMix)/5
		clear		gamixL, gamixR
endin

		instr		reverb
aL, aR		freeverb	gasendL, gasendR, gkRvbSize, 0.9
		outs		aL, aR
		clear		gasendL, gasendR
		endin

</CsInstruments>

<CsScore>
</CsScore>

</CsoundSynthesizer>
