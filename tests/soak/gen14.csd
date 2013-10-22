<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen14.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 
;after the example from The Csound Book, page 83

instr 1	;compare results from GEN13 & GEN14
		
iwshpfn	= p6	
inrmfn	= p7	
aswp	linseg	0.01, p3*.5, .49, p3*.5, 0.01		;index sweep function
aindex	poscil	aswp, p5, 2				;sound to waveshape
atable	tablei	aindex, iwshpfn, 1, .5			;waveshape index
anrm	tablei	aswp*2, inrmfn, 1			;normalization 
aenv	linen	p4, .01, p3, .02			;amplitude envelope
asig	= (atable*anrm)*aenv				;normalize and impose envelope
asig    dcblock2 asig					;get rid of DC
	outs    asig, asig

endin		
</CsInstruments>
<CsScore>

f 2 0 8192 10 1			;sine wave

f 28  0   4097 13  1 1 1 0 .8 0 .5 0 .2		;waveshaping function: GEN13 - odd harmonics
f 280 0   2049 4   28 1				;normalization function for f28
f 29  0   4097 14  1 1 1 0 .8 0 .5 0 .2		;waveshaping function: GEN14 - same harmonics
f 290 0   2049 4   29 1				;normalization function for f29


f 30  0   4097 13  1 1 0 1 0 .6 0 .4 0 .1	;waveshaping function: GEN13 - even harmonics
f 301 0   2049 4   30 1				;normalization function for f30
f 31  0   4097 14  1 1 0 1 0 .6 0 .4 0 .1	;waveshaping function: GEN13 - even harmonics
f 310 0   2049 4   31 1				;normalization function for 31
s
i1 0   3   .7   440 28  280 
i1 4   .   .7    .  29  290
i1 8   .   .7    .  30  301
i1 12  3   .7    .  31  310

e
</CsScore>
</CsoundSynthesizer>

