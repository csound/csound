<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc       -M0 ;;;RT audio I/O with MIDI in
</CsOptions>
<CsInstruments>

sr	=	44100
ksmps	=	100
nchnls	=	2

;Example by Gabriel Maldonado

giElem	init	8
giOutTab	ftgen	1,0,128, 2, 	0

              ;min1, max1, exp1, type1, min2, max2, exp2, type2, min3, max3, exp3, type3 etc.
giConfigTab ftgen	2,0,128,-2,         .1, 1000, -1, 3,      .1, 1000, -1, 3,     .1, 1000, -1, 3,     30, 2000, -1, 3, \
                                        .1, 5000, -1,  5,      .1, 5000, -1,  5,     .1, 5000, -1,  5,     .1, 5000, -1,  5
giSine	ftgen	3,0,256,10,	1

	FLpanel	"This Panel contains a Slider Bank",600,600
	FLslidBnk2 "mod1@mod2@mod3@amp@freq1@freq2@freq3@freqPo", giElem, giOutTab, giConfigTab, 400, 500, 100, 10
	FLpanel_end

	FLrun

	instr 1

kmodindex1 	init 0
kmodindex2 	init 0 
kmodindex3 	init 0 
kamp 		init 0 
kfreq1 		init 0 
kfreq2	  	init 0 
kfreq3	 	init 0 
kfreq4 		init 0


       vtable1k  giOutTab, kmodindex1 , kmodindex2, kmodindex3, kamp, kfreq1, kfreq2 , kfreq3, kfreq4

amod1	oscili	kmodindex1, kfreq1, giSine
amod2	oscili	kmodindex2, kfreq2, giSine
amod3	oscili	kmodindex3, kfreq3, giSine
aout	oscili	kamp,       kfreq4+amod1+amod2+amod3, giSine
	
	outs	aout, aout
	endin


</CsInstruments>
<CsScore>

i1 0 3600
f0 3600

</CsScore>
</CsoundSynthesizer>
