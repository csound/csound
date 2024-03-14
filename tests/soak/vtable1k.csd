<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
</CsOptions>
<CsInstruments>

sr	=	44100
ksmps	=	100
nchnls	=	2

giElem	init	13
giOutTab	ftgen	1,0,128, 2, 	0
giFreqTab	ftgen	2,0,128,-7, 	1,giElem, giElem+1
giSine	ftgen	3,0,256,10,	1

	FLpanel	"This Panel contains a Slider Bank",500,400
	FLslidBnk	"mod1@mod2@mod3@amp@freq1@freq2@freq3@freqPo", giElem, giOutTab, 360, 600, 100, 10
	FLpanel_end

	FLrun

	instr 1

kout1	init	0
kout2	init	0
kout3	init	0
kout4	init	0
kout5	init	0
kout6	init	0
kout7	init	0
kout8	init	0

vtable1k  giOutTab, kout1 , kout2, kout3, kout4, kout5 , kout6, kout7, kout8
kmodindex1= 	2 * db(kout1 * 80 )
kmodindex2=	2 * db(kout2 * 80 )
kmodindex3=	2 * db(kout3 * 80 )
kamp	=	50 * db(kout4 * 70 )
kfreq1	=	1.1 * octave(kout5 * 10)
kfreq2	=	1.1 * octave(kout6 * 10) 
kfreq3	=	1.1 * octave(kout7 * 10)
kfreq4	=	30 * octave(kout8 * 8)

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
