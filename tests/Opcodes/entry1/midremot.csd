<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o midremot.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>
sr	= 44100
kr	= 441
ksmps	= 100
nchnls = 2

massign 1,1 
massign 2,2 
massign 3,3 
massign 4,4 
massign 5,5 

ga1 init 0
ga2 init 0

gi1	sfload	   "19Trumpet.sf2" 

gi2	sfload	   "01hpschd.sf2" 

gi3	sfload	   "07AcousticGuitar.sf2" 

gi4	sfload	   "22Bassoon.sf2" 

gitab	ftgen	1,0,1024,10,1

midremot "192.168.1.100", "192.168.1.101", 1
midremot "192.168.1.100", "192.168.1.102", 2
midremot "192.168.1.100", "192.168.1.103", 3

midglobal "192.168.1.100", 5


	instr 1
	sfpassign	   0, gi1
ifreq	cpsmidi
iamp	ampmidi 10
inum	notnum
ivel	veloc
kamp	linsegr	1,1,1,.1,0
kfreq	init	1
a1,a2	sfplay	ivel,inum,kamp*iamp,kfreq,0,0
	outs	a1,a2
vincr ga1, a1*.5
vincr ga2, a2*.5
	endin

	instr 2
	sfpassign	0,    gi2
ifreq	cpsmidi
iamp	ampmidi 15
inum	notnum
ivel	veloc
kamp	linsegr	1,1,1,.1,0
kfreq	init	1
a1,a2	sfplay	ivel,inum,kamp*iamp,kfreq,0,0
	outs	a1,a2
vincr ga1, a1*.4
vincr ga2, a2*.4
	endin

	instr 3
	sfpassign	0,    gi3
ifreq	cpsmidi
iamp	ampmidi 10
inum	notnum
ivel	veloc
kamp	linsegr	1,1,1,.1,0
kfreq	init	1
a1,a2	sfplay	ivel,inum,kamp*iamp,kfreq,0,0
	outs	a1,a2
vincr ga1, a1*.5
vincr ga2, a2*.5
	endin

	instr 4
	sfpassign	0,    gi4
ifreq	cpsmidi
iamp	ampmidi 15
inum	notnum
ivel	veloc
kamp	linsegr	1,1,1,.1,0
kfreq	init	1
a1,a2	sfplay	ivel,inum,kamp*iamp,kfreq,0,0
	outs	a1,a2
vincr ga1, a1*.5
vincr ga2, a2*.5
	endin

instr	5
      kamp midic7 1,0,1	
      denorm ga1
      denorm ga2
aL, aR  reverbsc ga1, ga2, .9, 16000, sr, 0.5
        outs aL, aR
	ga1	=	0
     ga2   =     0
endin

</CsInstruments>
<CsScore>
; Score
f0  160
</CsScore>

</CsoundSynthesizer>