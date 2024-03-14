<CsoundSynthesizer>
<CsOptions>
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
</CsOptions>
<CsInstruments>

sr     = 44100
ksmps  = 10
nchnls = 2

;Example by Gabriel Maldonado 2007
giElem	init	8
giOutTab	ftgen	1,0,128, 2, 	0
giSine	ftgen	3,0,256,10,	1
giOutTab2	ftgen	4,0,128, 2, 	0
itab	ftgen	29, 0, 129, 5,  .002, 128, 1		;** exponential ascending curve for slider mapping
giExpTab	ftgen	30, 0, 129, -24, itab, 0, 1                    ;** rescaled curve for slider mapping
giConfigTab ftgen	2,0,128,-2,         	1, 	500,	-1, 	13, \
				     	1, 	500,	-1,  	13, \
				          1, 	500,	-1, 	13, \
 					1, 	5000,	-1, 	13, \
\
					1, 	1000, 	-1,  	5, \
					1, 	1000, 	-1,  	5, \
					1, 	1000, 	-1,  	5, \
					1, 	5000, 	-1,  	5

	FLpanel	"Multiple FM",600,600
	FLslidBnk2 "mod1@mod2@mod3@amp@freq1@freq2@freq3@freqPo", giElem, giOutTab2, giConfigTab, 400, 500, 100, 10
giHandle	FLslidBnkGetHandle 
	FLpanel_end

	FLrun
	instr 1
ktrig slider8table  1, giOutTab, 0, \ ;	ctl	min	max	init	func
	27,	1, 	500,	3,	-1,	\ ;1 repeat rate
	28,	1, 	500,	4,	-1,	\ ;2 random freq. amount
	29,	1, 	500,	1,	-1,	\ ;3 random amp. amount
	30,	1,	5000,	1,	-1,	\ ;4 number of concurrent loop points
\
	31,	1, 	1000,	1,	-1,	\;5 kloop1
	32,	1, 	1000,	1,	-1,	\;6 kloop2
	33,	1, 	1000,	1,	-1,	\;7 kloop3
	34,	1, 	1000,	1,	-1	;8 kloop4
kmodindex1	init	0
kmodindex2	init	0
kmodindex3	init	0
kamp	init	0
kfreq1	init	0
kfreq2	init	0
kfreq3	init	0
kfreq4	init	0
          vtable1k  giOutTab2, kmodindex1, kmodindex2, kmodindex3, kamp, kfreq1, kfreq2, kfreq3, kfreq4
;	 *kflag, *ihandle, *ifn, *startInd, *startSlid, *numSlid;
	FLslidBnk2Setk  ktrig, giHandle, giOutTab, 0, 0, giElem
printk2 kmodindex1
printk2 kmodindex2,10
printk2 kmodindex3,20
printk2 kamp,30
amod1	oscili	kmodindex1, kfreq1, giSine
amod2	oscili	kmodindex2, kfreq2, giSine
amod3	oscili	kmodindex3, kfreq3, giSine
aout	oscili	kamp,       kfreq4+amod1+amod2+amod3, giSine
	outs	aout, aout
	endin
</CsInstruments>

<CsScore>
i1 0 3600
</CsScore>

</CsoundSynthesizer>

