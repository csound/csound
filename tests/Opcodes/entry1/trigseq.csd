<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o trigseq.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giTimes	ftgen	91, 0, 128, -2,     1, 1/2, 1/2, 1/8, 1/8, 1/2,1/2, 1/16, 1/16, 1/16, 1/16, 1/16, 1/16, 1/16, 1/16; times
giSeq	ftgen 	90, 0, 128, -2,     1, 2,     .5, 3,    .25, 4,      .10, 5,       .05, 6 ;** sequence amplitude and freq-ratio bins
 
	
instr	1

icps	init	p4
iamp	init	.3

kloop	init	p5
initndx	init	p6
kloop2	init	p7
initndx2 init	p8
kdur	init	p9
iminTime init	p10
imaxTime init	p11
kampratio init  1
kfreqratio init 1

ktime_unit expseg iminTime,p3/8,iminTime,p3* 3/4,imaxTime,p3/8,imaxTime


;**ktrig	seqtime	ktime_unit, kstart, kloop, initndx, kfn_times 
;ktrig	seqtime	1/ktime_unit, 0,      15, 0,      giTimes	

ktrig	metro	ktime_unit

;****	trigseq	ktrig_in,  kstart,  kloop, initndx,  kfn_values, kout1 [, kout2, kout3, ....,  koutN] 
	trigseq	ktrig, 	0, 	kloop2,initndx2,   giSeq,      kampratio, kfreqratio
	
;atrig	= ktrig*10000
	schedkwhen ktrig, -1, -1, 3, 0, kdur, kampratio*iamp, kfreqratio*icps
;	schedkwhen ktrig, -1, -1, 2, 0, ktrig, kampratio*iamp, kfreqratio*icps
	endin

instr	2

icps	init	p4
iamp	init .2	

kloop	init	p5
initndx	init	p6
kloop2	init	p7
initndx2 init	p8
kdur	init	p9
iminTime init	p10
imaxTime init	p11
kampratio init  1
kfreqratio init 1

ktime_unit expseg iminTime,p3/8,iminTime,p3* 3/4,imaxTime,p3/8,imaxTime


;**ktrig	seqtime	ktime_unit, kstart, kloop, initndx, kfn_times 
ktrig	seqtime	1/ktime_unit, 0,      15, 0,      giTimes	

;ktrig	metro	ktime_unit

;****	trigseq	ktrig_in,  kstart,  kloop, initndx,  kfn_values, kout1 [, kout2, kout3, ....,  koutN] 
	trigseq	ktrig,      0, 	   kloop2, initndx2,   giSeq,    kampratio, kfreqratio
printk2 ktrig
;atrig	= ktrig*10000
;	schedkwhen ktrig, -1, -1, 2, 0, kdur, kampratio*iamp, kfreqratio*icps
	schedkwhen ktrig, -1, -1, 3, 0, ktrig, kampratio*iamp, kfreqratio*icps
endin

instr	3

print p3
kenv	expseg	 1.04, p3,.04
a1	foscili	p4*a(kenv-0.04), p5,1,1,kenv*5, 2
	outs	a1, a1
endin

</CsInstruments>
<CsScore>
f2 0 8192 10 1

;	icps	unused	unused	kloop2	initndx2 kdur iminTime	imaxTime

s

i1 0  6	100	0	0	5	0	.2	3	15
i1 8  6	150	0	0	4	1	.1	4	30
i1 16 6	200	0	0	5	3	.25	8	50  
i1 24 6	300	0	0	3	0	.1	1	30  

i2 32 6	100	0	0	5	0	.2	1	1
i2 40 6 150	0	0	4	1	.1	.5	.5
i2 48 6	200	0	0	5	3	.25	3	.5  
i2 56 6	300	0	0	5	0	.1	1	8  

e
</CsScore>
</CsoundSynthesizer>

