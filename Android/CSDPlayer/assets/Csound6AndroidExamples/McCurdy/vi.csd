;vi

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>

<CsInstruments>
sr 		    = 		48000
ksmps 		= 		48
nchnls 		= 		2
0dbfs 		= 		1


gasendL		init		0
gasendR		init		0
		seed		0
		alwayson	"read_channels"	
		alwayson	"rescale_controls"		
		alwayson	"trigger_6_notes"
		alwayson	"reverb"

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		(0.15-0.01)/(1-0.01),"slider1"
gkslider2	chnget		"slider2"
		chnset		(0-(-1))/(2-(-1)),"slider2"
gkslider3	chnget		"slider3"
		chnset		(1-0.01)/(2-0.01),"slider3"
gkslider4	chnget		"slider4"
		chnset		(1-0.5)/(10-0.5),"slider4"
gkslider5	chnget		"slider5"
		chnset		1,"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		0.6,"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		(0.85-0.5)/(1-0.5),"trackpad.y"
		endin

		instr		rescale_controls
gkDamping	scale		gkslider1,1,0.01
gkOctaveShift	scale		gkslider2,2,-1
gkSpread	scale		gkslider3,2,0.01
gkDensity	scale		gkslider4,10,0.5
gkNStrings	scale		gkslider5,6,1
gkRvbMix	scale		gktrackpadx,1,0
gkRvbSize	scale		gktrackpady,1,0.4
		endin

		instr		trigger_6_notes
icount		=		1
inotes		ftgen		0,0,-6,-2,40,45,50,55,59,64	; starting notes
loop:
inote		table		icount-1,inotes
		event_i		"i", "string", 0, (60*6)+25, inote, icount
		loop_le		icount,1,6,loop
krate		rspline		0.005, 0.15, 0.1, 0.2
ktrig		metro		krate*gkDensity
		if ktrig=1 then
		reinit 		update
		endif	
update:
aenv		expseg		0.0001, 0.02, 1, 0.2, 0.0001, 1, 0.0001
apluck		pinkish		aenv
		rireturn	
koct		randomi		5, 10, 2
gapluck		butlp		apluck, cpsoct(koct)
endin

		instr		string
adlt		rspline		50, 250, 0.03, 0.06
apluck		vdelay3		gapluck, adlt*gkSpread, 600
adtn		jspline		15, 0.002, 0.02
astring		wguide1		apluck, cpsmidinn(p4)*semitone(adtn)*octave(gkOctaveShift), (sr/2)*gkDamping, 0.9995
astring		dcblock		astring
kOnOff		=		(gkNStrings>=p5?1:0)	; check if this voice should be active
kOnOff		port		kOnOff,0.5		; smooth on/off amplitude switch for this voice
aOnOff		interp		kOnOff			; interpolate to create a-rate amplitude switch (prevents quantisation noise) 
kpan		rspline		0, 1, 0.1, 0.2
astrL, astrR	pan2		astring*aOnOff, kpan
		outs		astrL, astrR
gasendL		=		gasendL+(astrL*gkRvbMix)
gasendR		=		gasendR+(astrR*gkRvbMix)
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
