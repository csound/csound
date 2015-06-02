<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc for RT audio input as well 
; For Non-realtime ouput leave only the line below:
; -o genfarey.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; GENfarey creates table gidelta. 
; The table contains the delta values of Farey Sequence 7 (p5=7).
; They are used as Inter Onset Intervals (IOIs) or event durations.
; If p6 is set to 1 for IOI output then the length of the table (p3=-18) is -(|F_7| - 1)
; Remember that a negative sign is for non-power-of-2 table lengths.
; The negative sign in front of the GEN number prevents post-normalisation of its values.

gidelta ftgen 0,0,-18,"farey",7,1

; Use GENfarey with p6 set to 2 to generate the denominators of fractions of F_7 
; this is used in this example as factors to create a series of pitches:
gimult ftgen 0,0,-18,"farey",7,2

;-------- loop and trigger instrument 901 using a Farey Sequence polyrhythm
	  instr 1
kindx init 0
kindx2 init 0
ktrigger init 0
ktime_unit init p6
kstart init p4
kloop init p5
kinitndx init 0
kfn_times init gidelta
knote init 60
kbasenote init p8
ifundam init p7
ktrigger seqtime ktime_unit, kstart, kloop, kinitndx, kfn_times
  if (ktrigger > 0 ) then
     kpitch = cpspch(ifundam)
     kmult tab kindx2, gimult
     kpitch = kpitch * kmult
     knote = kbasenote + kmult
     event "i", 901,   0,   .4, .10, kpitch, kpitch * .9, 0.4,  5,   .75, .8,  1.0, .15, .0,  .125, .125, .25, .5,  1.0, .0, .0,  .0,  .0,  .125, .25, .25, .25, knote
     kindx = kindx + 1
     kindx = kindx % kloop
     kindx2 = kindx2 + 1
     kindx2 = kindx2 % kloop
  endif
endin

;------ basic 2 Operators FM algorithm ----------------
	instr 901
inotedur	=		p3
imaxamp		=		p4 ;ampdb(p4)
icarrfreq	=		p5
imodfreq	=		p6
ilowndx		=		p7
indxdiff	=		p8-p7
knote	        =		p27
aampenv		linseg	p9, p14*p3, p10, p15*p3, p11, p16*p3, p12, p17*p3, p13 
adevenv		linseg	p18, p23*p3, p19, p24*p3, p20, p25*p3, p21, p26*p3, p22
amodosc		oscili	(ilowndx+indxdiff*adevenv)*imodfreq, imodfreq, 10 
acarosc		oscili	imaxamp*aampenv, icarrfreq+amodosc, 10 
		outs		acarosc, acarosc  
endin
</CsInstruments>
<CsScore>
f10 0 4096 10 1	;sine wave			
; p4 kstart  := index offset into the Farey Sequence
; p5 kloop   := end index into Farey Seq.
; p6 timefac := time in seconds for one loop to complete
; p7 fundam  := fundamental of the FM instrument
; p8 basenote:= root pitch of the midi voice output
; note that pitch structures of the midi file output are not equivalent to the
; ones used for the FM real-time synthesis.

;	start		dur		kstart	kloop   timefac	fundam. basenote
i1	0.0		44		0 	18	2	6.05	60
i1	4		30		0 	18	3	7.05	72
i1	34		12		9 	18	3	7.05	72
i1	10		12		0 	18	1.5	8	84
i1	22		12		0 	9	1.5	8	84
i1	15		16		0	18	1	5	48
i1	22		20		5	17	1.7	4	36

i1	46		20		3 	11	2.5	7.04	71
i1	51		20		5 	13	2.5	7.06	72

i1	73.5		1.5		11	18	1.5	5.05	48
i1	75		1		12	18	1	6.03	58	
e
</CsScore>
</CsoundSynthesizer>

