<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=alsa --midioutfile=farey7.mid
</CsOptions>
<CsInstruments>
sr=48000
ksmps=10
nchnls=1
0dbfs = 1

gidelta init 100
gimult init 101

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
     event "i", 901, 0, .4, .1, kpitch, kpitch * .9, .4,  5,   .75, .8,  1.0, .15, .0,  .125, .125, .25, .5,  1.0, .0, .0,  .0,  .0,  .125, .25, .25, .25, knote
     kindx = kindx + 1
     kindx = kindx % kloop
     kindx2 = kindx2 + 1
     kindx2 = kindx2 % kloop
     if (kindx2 == 0) then
     	tableshuffle gimult
     endif
	
endif
	endin ; 1

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
			out		acarosc 
;------ we also write down a midi track here ----------
midion 1, knote, 100
       	endin ; 901

</CsInstruments>

<CsScore>
f10 0 4096 10 1				
f100 0 -18 "farey" 7 1
f101 0 -18 "farey" 7 2

; p4 kstart  := index offset into the Farey Sequence
; p5 kloop   := end index into Farey Seq.
; p6 timefac := time in seconds for one loop to complete
; p7 fundam  := fundamental of the FM instrument
; p8 basenote:= root pitch of the midi voice output
; note that pitch structures of the midi file output are not equivalent to the
; ones used for the FM real-time synthesis.

;	start		dur		kstart	kloop   timefac	fundam. basenote
i1	0.0		44		0 	18	1	6.05	60
i1	4		40		0 	18	3	7.05	72
i1	10		38		0 	18	1.5	8	84
i1	15		50		0	18	1	5	48
i1	22		75		5	17	1.7	4	36	
e
</CsScore>

</CsoundSynthesizer>
