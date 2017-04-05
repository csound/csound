;v

<CsoundSynthesizer>

<CsOptions>
-odac -dm3
</CsOptions>

<CsInstruments>
sr 		    =	 	48000
ksmps 		= 		48
nchnls 		= 		2
0dbfs 		= 		1

gisine		ftgen		0, 0, 4096, 10, 1
gasendL		init		0
gasendR		init		0
		seed		0
		alwayson	"read_channels"		
		alwayson	"rescale_controls"		
		alwayson	"start_sequences"
		alwayson	"reverb"

		instr		read_channels
gkslider1	chnget		"slider1"
		chnset		(1-0.25)/(4-0.25),"slider1"
gkslider2	chnget		"slider2"
		chnset		(1-0.5)/(2-0.5),"slider2"
gkslider3	chnget		"slider3"
		chnset		(1-0.25)/(4-0.25),"slider3"
gkslider4	chnget		"slider4"
		chnset		(1-0.25)/(32-0.25),"slider4"
gkslider5	chnget		"slider5"
		chnset		(1-0.1)/(4-0.1),"slider5"
gktrackpadx	chnget		"trackpad.x"
		chnset		0,"trackpad.x"
gktrackpady	chnget		"trackpad.y"
		chnset		1,"trackpad.y"
		endin

		instr		rescale_controls
gkSpeed		scale		gkslider1,4,0.25
gksep		scale		gkslider2,4,0.25
gkFreq		scale		gkslider3,4,0.25
kporttime	linseg		0,0.01,0.01
gkFreq		portk		gkFreq,kporttime
gkRateOfChange		scale		gkslider4,32,0.125
gkQ		scale		gkslider5,2,0.5
gkminfb		scale		gktrackpadx,1,0
gkmaxfb		scale		gktrackpady,1,0.98
		endin

		instr		start_sequences
irate		random		1, 2.5
		event_i		"i", "sound_instr",           0, 300, irate, 0.9, 0.03, 0.06, 8, 0.5, 1
		event_i		"i", "sound_instr", 1/(2*irate), 300, irate, 0.9, 0.03, 0.06, 8, 0.5, 1
		event_i		"i", "sound_instr", 1/(4*irate), 300, irate, 0.9, 0.03, 0.06, 8, 0.5, 1
		event_i		"i", "sound_instr", 3/(4*irate), 300, irate, 0.9, 0.03, 0.06, 8, 0.5, 1
ktrig		metro		(irate*gkSpeed)/64
		schedkwhennamed	ktrig, 0, 0, "sound_instr", 1/(irate), 64/irate, irate/16, 0.996, 0.003, 0.01, 3, 0.7, 1
		schedkwhennamed	ktrig, 0, 0, "sound_instr", 2/(irate), 64/irate, irate/16, 0.996, 0.003, 0.01, 4, 0.7, 1
ktrig		metro		(irate*gkSpeed)/72
		schedkwhennamed	ktrig, 0, 0, "sound_instr", 3/(irate), 72/irate, irate/20, 0.996, 0.003, 0.01, 5, 0.7, 1
		schedkwhennamed	ktrig, 0, 0, "sound_instr", 4/(irate), 72/irate, irate/20, 0.996, 0.003, 0.01, 6, 0.7, 1
iminfb		ftgen		0,0,4096,-16,0,4096,-12,0.999
gkminfb2	tablei		gkminfb*(ftlen(iminfb)-1),iminfb
gkmaxfb		=		(gkminfb>gkmaxfb?gkminfb:gkmaxfb)
		endin

		instr		sound_instr
ktrig		metro		p4*gkSpeed
		if ktrig=1 then
		reinit 		PULSE
		endif
PULSE:
ioct		random		7.3, 10.5
icps		init		cpsoct(ioct)
aptr		linseg		0, 1/icps, 1
		rireturn
a1		tablei		aptr, gisine, 1	
kamp		rspline		0.2, 0.7, 0.1, 0.8
a1		=		a1*(kamp^3)
kphsoct		rspline		6, 10, p6*gkRateOfChange, p7*gkRateOfChange
isep		random		0.5, 0.75
ksep		transeg		isep+1, 0.02, -50, isep
kfeedback	rspline		0.85, 0.99, 0.01, 0.1
kfb		limit		p5,gkminfb2,gkmaxfb
aphs2		phaser2		a1, cpsoct(kphsoct)*gkFreq, 0.3*gkQ, p8, p10, isep*gksep, kfb
aDlyMod		oscili		0.0005,0.7,gisine
acho		vdelay3		aphs2+a1, (aDlyMod+0.0005+0.0001)*1000,1000
aphs2		sum		aphs2, acho
aphs2		butlp		aphs2, 2000
kenv		linseg		1, p3-4, 1, 4, 0
kpan		rspline		0, 1, 0.1, 0.8
kattrel		linsegr		1, 1, 0
a1, a2		pan2		aphs2*kenv*p9*kattrel, kpan
a1		delay		a1, rnd(0.01)+0.0001
a2		delay		a2, rnd(0.01)+0.0001
ksend		rspline		0.2, 0.7, 0.05, 0.1
ksend		=		abs(ksend)^2
		outs		a1*(1-ksend), a2*(1-ksend)
gasendL		=		gasendL+(a1*ksend*1.5)
gasendR		=		gasendR+(a2*ksend*1.5)
		endin

		instr		reverb
aL, aR		freeverb	gasendL, gasendR, 0.7, 0.9
		outs		aL, aR
		clear		gasendL, gasendR
		endin

</CsInstruments>

<CsScore>
f 0 3600
e
</CsScore>

</CsoundSynthesizer>
