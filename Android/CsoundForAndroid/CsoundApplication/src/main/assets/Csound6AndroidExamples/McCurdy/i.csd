;i
gkNVoices	1		8		1		6		linear		Number of simulataneous voices
gkOctaveShift	-1		1		-		0		linear		transposition in octaves
gkBrightness	0.1		2		-		1		linear		brightness
gkLevel		0		1		-		0.7		linear		output level
gkSpeed		1		4		-		1		linear		speed of the glissandi
gkrange		7		72		-		54		linear		range for random pitch choice for new notes 
gkbase		12		72		-		12		linear		base offset for random pitch choice for new notes 

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>

<CsInstruments>
sr 		= 		48000
ksmps 		= 		48
nchnls 		= 		2
0dbfs 		= 		1

gisine		ftgen		0, 0, 131072, 10, 1
gasendL,gasendR		init		0
		seed		0
		alwayson	"read_channels"		 
		alwayson	"rescale_controls"		
		alwayson	"start_multiple_long_notes"
		alwayson	"reverb"

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		(6-1)/(8-1),"slider1"
gkslider2	chnget		"slider2"
		chnset		0.5,"slider2"
gkslider3	chnget		"slider3"
		chnset		(1-0.1)/(2-0.1),"slider3"
gkslider4	chnget		"slider4"
		chnset		0,"slider4"
gkslider5	chnget		"slider5"
		chnset		0,"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		(54-12)/(72-12),"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		0,"trackpad.y"
		endin
		
		instr		rescale_controls
gkNVoices	scale		gkslider1,8,1
gkOctaveShift	scale		gkslider2,1,-1
gkBrightness	scale		gkslider3,2,0.1
gklh		scale		gkslider4,11,1
gkSpeed		scale		gkslider5,4,1
gkrange		scale		gktrackpadx,72,7
gkbase		scale		gktrackpady,72,12
		endin

		instr		start_multiple_long_notes
icount		=		1
loop:		
		event_i		"i", "trombone", icount-0.9, 3600
		loop_le		icount,1,8,loop
		endin

		instr		trombone
knote		init		rnd(12)+54
knote		=		int(knote)
ktrig		metro		0.015*gkSpeed
		if(ktrig==1) then
		reinit 		retrig
		endif
retrig:
inote1		init		i(knote)
inote2		init		rnd(i(gkbase))+i(gkrange)
inote2		limit		int(inote2),12,84
inotemid	=		inote1+((inote2-inote1)/2)
idur		=		(22.5+rnd(5))/i(gkSpeed)
icurve		=		2
		timout		0, idur, skip
knote		transeg		inote1,idur/2,icurve,inotemid,idur/2,-icurve,inote2	
skip:
		rireturn
kenv		linseg		0, 25, 0.03, p3-50, 0.03, 25, 0
kdtn		jspline		0.05, 0.4, 0.8		; slowly shifting detuning
kmul		rspline		0.3, 0.82, 0.04, 0.2	; brightness control
kamp		rspline		0.02, 3, 0.05, 0.1	; random amplitude
kmul		limit		(kmul^1.75)*gkBrightness,0,0.9
a1		gbuzz		kenv*kamp, cpsmidinn(knote)*semitone(kdtn)*octave(gkOctaveShift), 75, gklh, kmul, gisine
a1		dcblock2	a1
kOnOff		=		(gkNVoices>=p2?1:0)	; check if this voice should be active
kOnOff		port		kOnOff,0.5		; smooth on/off amplitude switch for this voice
aOnOff		interp		kOnOff			; interpolate to create a-rate amplitude switch (prevents quantisation noise) 
kpan		rspline		0,1,0.1,1		; random panning movement
a1, a2		pan2		a1*aOnOff, kpan		; create stereo auto-panning signal
		outs		a1, a2			; send audio to outputs
gasendL		=		gasendL+a1		; add audio to reverb send signal
gasendR		=		gasendR+a2
		endin

		instr		reverb
aL, aR		reverbsc	gasendL,gasendR,0.85,10000
		outs		aL, aR
		clear		gasendL, gasendR
		endin
		
</CsInstruments>

<CsScore>
</CsScore>

</CsoundSynthesizer>
