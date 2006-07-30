<CsoundSynthesizer>
<CsOptions>
csound -h -n -f -d -M0 -m7 -+rtmidi=null --midi-key=4 --midi-velocity=5 temp.orc temp.sco
</CsOptions>
<CsInstruments>
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; T H E   S I L E N C E   O R C H E S T R A
; Copyright (c) 2006 by Michael Gogins
; This file is licensed under the GNU Lesser General Public License
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; OBJECTIVES
;
; - Most beautiful sounds
; - Highest precision
; - Lowest noise
; - No clicks
; - MIDI/offline interoperability
; - Gains normalized across instruments, pitches, velocities
; - Modular code
;
; PFIELDS
;
; All instruments use the following standardized set of pfields:
;
; p1 	Instrument number
; p2    Time of note, in absolute seconds from start of performance
; p3 	Duration of note, in seconds
; p4 	MIDI key (may be fractional)
; p5	MIDI velocity, interpreted as decibels up (may be fractional)
; p6	Audio phase, in radians (seldom used; enables grain notes to
;	implement arbitrary audio transforms)
; p7	x location or stereo pan (-1 through 0 to +1)
; p8	y location or stage depth (-1 through 0 to +1)
; p9	z location or stage height (-1 through 0 to +1)
; p10	Pitch-class set, as sum of 2^(pitch-class).
;
; EFFECTS BUSSES
;
; The orchestra uses one input buss for each of the following effects:
; 
; Leslie
; Chorus
; Reverberation
; Output
;
; MASTER OUTPUT EFFECTS
; 
; The master output buss has the following additional effects:
;
; Bass enhancement
; Compression
; Remove DC bias
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; O R C H E S T R A   H E A D E R
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sr                      =                       44100
ksmps			=                       15
nchnls                  =                       2
; Note that -1 dB for float samples is 29205.
0dbfs                   =                       32767.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; A S S I G N   M I D I   C H A N N E L S   T O   I N S T R U M E N T S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

			massign			1,  6
			; massign			 3, 12
			; massign			 4, 37
			; massign			 5, 11
			; massign			 6,  9
			; massign			 7, 51
			; massign			 8, 52
			; massign			 9, 53
			; massign			10, 15
			; massign			11, 10
			; massign			12, 21
			; massign			13, 22
			; massign			14, 28
			; massign			15, 26
			; massign			16, 41

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; F U N C T I O N   T A B L E S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

			; Waveform for the string-pad
giwave                  ftgen                   1, 0, 65537,    10,     1, .5, .33, .25,  .0, .1,  .1, .1
gisine                  ftgen                   2, 0, 65537,    10,     1
giharpsichord           ftgen                   0, 0, 65537,     7,	-1, 1024, 1, 1024, -1 ; Kelley harpsichord.
gicosine                ftgen                   0, 0, 65537,    11,     1 ; Cosine wave. Get that noise down on the most widely used table!
giexponentialrise       ftgen                   0, 0, 65537,     5,     .001, 513, 1 ; Exponential rise.
githirteen              ftgen                   0, 0, 65537,     9,     1, .3, 0
giln                    ftgen                   0, 0, 65537,   -12,    20.0 ; Unscaled ln(I(x)) from 0 to 20.0.
gibergeman              ftgen                   0, 0, 65537,    10,     .28, 1, .74, .66, .78, .48, .05, .33, .12, .08, .01, .54, .19, .08, .05, .16, .01, .11, .3, .02, .2 ; Bergeman f1
gicookblank             ftgen                   0, 0, 65537,    10,     0 ; Blank wavetable for some Cook FM opcodes.
gicook3                 ftgen                   0, 0, 65537,    10,     1, .4, .2, .1, .1, .05
gikellyflute            ftgen                   0, 0, 65537,    10,     1, .25, .1 ; Kelley flute.
gichebychev             ftgen                   0, 0, 65537,    -7,    -1, 150, .1, 110, 0, 252, 0
giffitch1               ftgen                   0, 0, 65537,    10,     1
giffitch2               ftgen                   0, 0, 65537,     5,     1, 1024, .01
giffitch3               ftgen                   0, 0, 65537,     5,     1, 1024, .001
			; Rotor Tables
gitonewheel1            ftgen                   0, 0, 65537,    10,     1, .02, .01
gitonewheel2            ftgen                   0, 0, 65537,    10,     1, 0, .2, 0, .1, 0, .05, 0, .02
			; Rotating Speaker Filter Envelopes
gitonewheel3            ftgen                   0, 0, 65537,     7,     0, 110, 0, 18, 1, 18, 0, 110, 0
gitonewheel4            ftgen                   0, 0, 65537,     7,     0, 80, .2, 16, 1, 64, 1, 16, .2, 80, 0
			; Distortion Tables
gitonewheel5            ftgen                   0, 0, 65537,     8,    -.8, 336, -.78,  800, -.7, 5920, .7,  800, .78, 336, .8
gitonewheel6            ftgen                   0, 0, 65537,     8     -.8, 336, -.76, 3000, -.7, 1520, .7, 3000, .76, 336, .8

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; S O U N D F O N T   A S S I G N M E N T S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


giFluidsynth		fluidEngine		0, 0
giFluidSteinway		fluidLoad		"/utah/home/mkg/projects/music/library/soundfonts/Piano Steinway Grand Model C (21,738KB).sf2",  giFluidsynth, 1
			fluidProgramSelect	giFluidsynth, 0, giFluidSteinway, 0, 1

giFluidGM		fluidLoad		"/utah/home/mkg/projects/music/library/soundfonts/63.3mg The Sound Site Album Bank V1.0.SF2", giFluidsynth, 1
			fluidProgramSelect	giFluidsynth, 1, giFluidGM, 0, 26

giFluidMarimba		fluidLoad		"/utah/home/mkg/projects/music/library/soundfonts/Marimba Moonman (414KB).SF2", giFluidsynth, 1
			fluidProgramSelect	giFluidsynth, 2, giFluidMarimba, 0, 0

giFluidOrgan		fluidLoad		"/utah/home/mkg/projects/music/library/soundfonts/Organ Jeux V1.4 (3,674KB).SF2", giFluidsynth, 1
			fluidProgramSelect	giFluidsynth, 3, giFluidOrgan, 0, 36
						
			
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; U S E R - D E F I N E D   O P C O D E S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
						
			opcode 			AssignSend, 0, iiiii
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
inso,ic,il,ir,im 	xin
inum			init			floor(inso)
			MixerSetLevel	 	inum, 200, ic
			MixerSetLevel	 	inum, 201, il
			MixerSetLevel	 	inum, 210, ir
			MixerSetLevel	 	inum, 220, im
			endop
                        
			opcode			NoteOn, ii, iii
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        
ikey,ivelocity,imeasure xin
inormal			=			ampdb(80)
                        ; MIDI key number to linear octave.
ikey                     =                       ikey / 12.0 + 3.0
                        ; Linear octave to frequency in Hertz.
ifrequency 		= 			cpsoct(ikey)
			; Normalize so iamplitude for p5 of 80 == ampdb(80) == 10000.
iamplitude 		= 			ampdb(ivelocity) * (inormal / imeasure)
 			xout			ifrequency, iamplitude
			endop
                        
			opcode			SendOut, 0, aa
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
insno, aleft, aright	xin
inum			init			floor(insno)
			MixerSend               aleft, inum, 200, 0
			MixerSend               aright, inum, 200, 1
			MixerSend               aleft, inum, 201, 0
			MixerSend               aright, inum, 201, 1
			MixerSend               aleft, inum, 210, 0
			MixerSend               aright, inum, 210, 1
			MixerSend               aleft, inum, 220, 0
			MixerSend               aright, inum, 220, 1
			endop

			opcode			Pan, aa, ia
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ipan,asignal		xin
			; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       ipan * ipi / 2.0
itheta                  =                       iradians / 2.0
			; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
			xout			ileftgain * asignal, irightgain * asignal
			endop

			opcode			Declick, aa, iiiaa
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
iatt,idur,irel,a1,a2	xin
                        if (idur > 0)             then
isustain		= 			idur
idur			=			iatt + isustain + irel                        
                        else
isustain                =                       100000
                        endif                        
aenv			linsegr			0.0, iatt, 1.0, isustain, 1.0, irel, 0.0
ab1			=			a1 * aenv
ab2			=			a2 * aenv
			xout			idur, ab1, ab2
			endop	


			opcode			Damping, ia, iii
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
idur,iatt,irel		xin
                        if (idur > 0)           then
isustain		= 			idur
idur			=			iatt + isustain + irel                        
                        else
isustain                =                       100000
                        endif                        
                        ; Releasing envelope for MIDI performance.
aenv			linsegr			0.0, iatt, 1.0, isustain, 1.0, irel, 0.0
			xout			idur, aenv
			endop						
			
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; M I X E R   L E V E L S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

			instr                   1 ; Mixer level
isend                   =                       p4
ibuss0                  =                       p5
igain0                 	=                       p6
			MixerSetLevel           isend, ibuss0, igain0
			endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; I N S T R U M E N T   D E F I N I T I O N S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

			instr                   2 ; Xanadu instr 1
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500000
p3,adamping		Damping			.003, p3,.1
ishift			=           		8./1200.
ipch        		=           		ifrequency              	; convert parameter 5 to cps.
ioct        		=           		octcps(ifrequency)      	; convert parameter 5 to oct.
kvib        		poscil			1./120., ipch/50., gicosine	; vibrato
ag          		pluck       		2000, cpsoct(ioct + kvib),   ipch, 1, 1
agleft      		pluck       		2000, cpsoct(ioct + ishift), ipch, 1, 1
agright     		pluck       		2000, cpsoct(ioct - ishift), ipch, 1, 1
af1         		expon       		.01, 10., 1.0             	; exponential from 0.1 to 1.0
af2         		expon       		.015, 15., 1.055             	; exponential from 1.0 to 0.1
adump       		delayr      		2.0                     	; set delay line of 2.0 sec
atap1       		deltap3     		af1                     	; tap delay line with kf1 func.
atap2       		deltap3     		af2                     	; tap delay line with kf2 func.
ad1         		deltap3      		2.0                     	; delay 2 sec.
ad2         		deltap3      		1.1                     	; delay 1.1 sec.
			delayw      		ag * adamping                  	; put ag signal into delay line.
aleft 			= 			agleft+atap1+ad1
aright			=			agright+atap2+ad2
aleft, aright		Pan			p7,iamplitude * (aleft + aright) * adamping
			AssignSend		p1,0., 0., .2, 1.
			SendOut			p1, aleft, aright
			endin

			instr                   3 ; Xanadu instr 2
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500000
p3,adamping		Damping			.01, p3, .01
ishift      		=           		8./1200.
ipch       		=           		ifrequency
ioct        		=           		octcps(ifrequency) 
kvib        		poscil       		1./80., 6.1, gicosine      	; vibrato
ag          		pluck       		1000, cpsoct(ioct + kvib),   ipch, 1, 1
agleft      		pluck       		1000, cpsoct(ioct + ishift), ipch, 1, 1
agright     		pluck       		1000, cpsoct(ioct - ishift), ipch, 1, 1
adump       		delayr      		0.4                     	; set delay line of 0.3 sec
ad1         		deltap3      		0.07                     	; delay 100 msec.
ad2         		deltap3      		0.105                    	; delay 200 msec.
			delayw      		ag * adamping                  	; put ag sign into del line.
aleft			=			agleft + ad1
aright			=			agright + ad2
aleft, aright		Pan			p7, iamplitude * (aleft + aright) * adamping
			AssignSend		p1, 0, 0, .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   4 ; Xanadu instr 3
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500000
p3, adamping		Damping			.01, p3, .01
ishift      		=           		8. / 1200.
ipch        		=           		ifrequency
ioct        		=           		octcps(ifrequency)
; kadsr       		linseg      		0, p3/3, 1.0, p3/3, 1.0, p3/3, 0 	; ADSR envelope
amodi       		linseg      		0, p3/3, 5, p3/3, 3, p3/3, 0 		; ADSR envelope for I
ip6			=			1.4
ip7			=			0.8
amodr       		linseg      		ip6, p3, ip7              		; r moves from p6->p7 in p3 sec.
a1          		=           		amodi*(amodr-1/amodr)/2
a1ndx       		=           		abs(a1*2/20)            		; a1*2 is normalized from 0-1.
a2          		=           		amodi*(amodr+1/amodr)/2
a3          		tablei      		a1ndx, giln, 1             		; lookup tbl in f3, normal index
ao1         		poscil       		a1, ipch, gicosine             
a4          		=           		exp(-0.5*a3+ao1)
ao2         		poscil       		a2*ipch, ipch, gicosine        
;aoutl       		poscil       		1000*kadsr*a4, ao2+cpsoct(ioct+ishift), gisine 
;aoutr       		poscil       		1000*kadsr*a4, ao2+cpsoct(ioct-ishift), gisine 
aoutl       		poscil       		1000*a4, ao2+cpsoct(ioct+ishift), gisine 
aoutr       		poscil       		1000*a4, ao2+cpsoct(ioct-ishift), gisine 
aleft			=			aoutl * iamplitude * adamping
aright			=			aoutr * iamplitude * adamping
; aleft, aright		Pan			p7, aleft + aright
			AssignSend		p1, 0., 0., .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   5 ; Tone wheel organ by Mikelson
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 20000
iphase			=			0.
ikey                    =                       12 * int(p4 - 6) + 100 * (p4 - 6)
ifqc                    =                       ifrequency
			; The lower tone wheels have increased odd harmonic content.
iwheel1                 =                       ((ikey - 12) > 12 ? gitonewheel1 : gitonewheel2)
iwheel2                 =                       ((ikey +  7) > 12 ? gitonewheel1 : gitonewheel2)
iwheel3                 =                        (ikey       > 12 ? gitonewheel1 : gitonewheel2)
iwheel4                 =                       1
			;  Start Dur   Amp   Pitch SubFund Sub3rd Fund 2nd 3rd 4th 5th 6th 8th
			;i1   0    6    200    8.04   8       8     8    8   3   2   1   0   4
asubfund                poscil                  8, .5*ifqc,      iwheel1, iphase/(ikey-12)
asub3rd                 poscil                  8, 1.4983*ifqc,  iwheel2, iphase/(ikey+7)
afund                   poscil                  8, ifqc,         iwheel3, iphase/ikey
a2nd                    poscil                  8, 2*ifqc,       iwheel4, iphase/(ikey+12)
a3rd                    poscil                  3, 2.9966*ifqc,  iwheel4, iphase/(ikey+19)
a4th                    poscil                  2, 4*ifqc,       iwheel4, iphase/(ikey+24)
a5th                    poscil                  1, 5.0397*ifqc,  iwheel4, iphase/(ikey+28)
a6th                    poscil                  0, 5.9932*ifqc,  iwheel4, iphase/(ikey+31)
a8th                    poscil                  4, 8*ifqc,       iwheel4, iphase/(ikey+36)
asignal                 =                       asubfund + asub3rd + afund + a2nd + a3rd + a4th + a5th + a6th + a8th
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.005, p3, 0.3, aleft, aright
			AssignSend		p1, 0., 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   6 ; Guitar, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 400
aenvelope               transeg                 1.0, 10.0, -5.0, 0.0
asigcomp                pluck                   1, 440, 440, 0, 1
kbend                   cpsmidib
asig                    pluck                   1, kbend + ifrequency, ifrequency, 0, 1
af1                     reson                   asig, 110, 80
af2                     reson                   asig, 220, 100
af3                     reson                   asig, 440, 80
asignal                 balance                 0.6 * af1+ af2 + 0.6 * af3 + 0.4 * asig, asigcomp
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.01, p3, 0.3, aleft, aright
			AssignSend		p1, 0., 0.,  .4, 1
			SendOut			p1, aleft, aright
			endin

			instr                   7 ; Harpsichord, James Kelley
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 600
aenvelope               transeg                 1.0, 10.0, -5.0, 0.0
apluck                  pluck                   iamplitude, ifrequency, ifrequency, 0, 1
aharp                   poscil                  aenvelope, ifrequency, giharpsichord
aharp2                  balance                 apluck, aharp
asignal			=			apluck + aharp2
aleft, aright		Pan			p7, asignal
p3, aleft, aright	Declick			.005, p3, 0.3, aleft, aright
			AssignSend		p1, 0., 0., .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   8 ; Heavy metal model, Perry Cook
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 100
iindex                  =                       1
icrossfade              =                       3
ivibedepth              =                       0.02
iviberate               =                       4.8
ifn1                    =                       gisine
ifn2                    =                       giexponentialrise
ifn3                    =                       githirteen
ifn4                    =                       gisine
ivibefn                 =                       gicosine
adecay                  transeg                 0.0, .001, 4, 1.0, 2.0, -4, 0.1, 0.125, -4, 0.0
asignal                 fmmetal                 0.1, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.005, p3, 0.3, aleft, aright
			AssignSend		p1, 0., 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   9 ; Xing by Andrew Horner
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
			; p4 pitch in octave.pch
			; original pitch        = A6
			; range                 = C6 - C7
			; extended range        = F4 - C7
ifrequency,iamplitude	NoteOn                  p4, p5, 300
isine                   =                       1
iinstrument             =                       p1
istarttime              =                       p2
ioctave                 =                       p4
idur                    =                       p3
ifreq                   =                       ifrequency
iamp                    =                       1
inorm                   =                       32310
aamp1                   linseg                  0,.001,5200,.001,800,.001,3000,.0025,1100,.002,2800,.0015,1500,.001,2100,.011,1600,.03,1400,.95,700,1,320,1,180,1,90,1,40,1,20,1,12,1,6,1,3,1,0,1,0
adevamp1                linseg                  0, .05, .3, idur - .05, 0
adev1                   poscil                  adevamp1, 6.7, gisine, .8
amp1                    =                       aamp1 * (1 + adev1)
aamp2                   linseg                  0,.0009,22000,.0005,7300,.0009,11000,.0004,5500,.0006,15000,.0004,5500,.0008,2200,.055,7300,.02,8500,.38,5000,.5,300,.5,73,.5,5.,5,0,1,1
adevamp2                linseg                  0,.12,.5,idur-.12,0
adev2                   poscil                  adevamp2, 10.5, gisine, 0
amp2                    =                       aamp2 * (1 + adev2)
aamp3                   linseg                  0,.001,3000,.001,1000,.0017,12000,.0013,3700,.001,12500,.0018,3000,.0012,1200,.001,1400,.0017,6000,.0023,200,.001,3000,.001,1200,.0015,8000,.001,1800,.0015,6000,.08,1200,.2,200,.2,40,.2,10,.4,0,1,0
adevamp3                linseg                  0, .02, .8, idur - .02, 0
adev3                   poscil                  adevamp3, 70, gisine ,0
amp3                    =                       aamp3 * (1 + adev3),
awt1                    poscil                  amp1, ifreq, gisine
awt2                    poscil                  amp2, 2.7 * ifreq, gisine
awt3                    poscil                  amp3, 4.95 * ifreq, gisine
asig                    =                       awt1 + awt2 + awt3
arel                    linenr                  1,0, idur, .06
asignal                 =                       asig * arel * (iamp / inorm)
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.005, p3, 0.3, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr                   10 ; FM modulated left and right detuned chorusing, Thomas Kung
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 5530
iattack 		= 			0.25
isustain		=			p3
irelease 		= 			0.3333333
p3, adamping		Damping			iattack,p3,irelease
ip6                     =                       0.3
ip7                     =                       2.2
ishift      		=           		4./12000.
ipch       		=           		ifrequency
ioct        		=           		octcps(ifrequency) 
aadsr                   linen                   1.0, iattack, irelease, 0.01
amodi                   linseg                  0, iattack, 5, p3, 2, irelease, 0
			; r moves from ip6 to ip7 in p3 secs.
amodr                   linseg                  ip6, p3, ip7
a1                      =                       amodi * (amodr - 1 / amodr) / 2
			; a1*2 is argument normalized from 0-1.
a1ndx                   =                       abs(a1 * 2 / 20)
a2                      =                       amodi * (amodr + 1 / amodr) / 2
			; Look up table is in f43, normalized index.
a3                      tablei                  a1ndx, giln, 1
			; Cosine
ao1                     poscil                  a1, ipch, gicosine
a4                      =                       exp(-0.5 * a3 + ao1)
			; Cosine
ao2                     poscil                  a2 * ipch, ipch, gicosine
			; Final output left
aleft                   poscil                  a4, ao2 + cpsoct(ioct + ishift), gisine
			; Final output right
aright                  poscil                  a4, ao2 + cpsoct(ioct - ishift), gisine
aleft, aright		Pan			p7, (aleft + aright) * iamplitude
			AssignSend		p1, 0., 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   11 ; String pad
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			; String-pad borrowed from the piece "Dorian Gray",
			; http://akozar.spymac.net/music/ Modified to fit my needs
ihz, iamp		NoteOn                  p4, p5, 1000.
			; Slow attack and release
actrl   		linseg  		0, p3*0.5, 1.0, p3*.5, 0
			; Slight chorus effect
afund   		poscil   		actrl, ihz,  giwave       	; audio oscillator
acel1   		poscil   		actrl, ihz - .1, giwave       	; audio oscilator - flat
acel2   		poscil   		actrl, ihz + .1, giwave       	; audio oscillator - sharp
asig    		=   			afund + acel1 + acel2
			; Cut-off high frequencies depending on midi-velocity
			; (larger velocity implies brighter sound)
;asig 			butterlp 		asig, 900 + iamp / 40.
aleft, aright		Pan			p7, asig * iamp
p3, aleft, aright	Declick			.25, p3, .5, aleft, aright
			AssignSend		p1, .2, 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   12 ; Filtered chorus, Michael Bergeman
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 4000
ioctave			=			octcps(ifrequency)
idb			= 			1.5
ip5                     =                       gibergeman
ip3                     =                       p3
ip6                     =                       p3 * .25
ip7                     =                       p3 * .75
ip8                     =                       cpsoct(ioctave - .01)
ip9                     =                       cpsoct(ioctave + .01)
isc                     =                       idb * .333
k1                      line                    40, p3, 800
k2                      line                    440, p3, 220
k3                      linen                   isc, ip6, p3, ip7
k4                      line                    800, ip3,40
k5                      line                    220, ip3,440
k6                      linen                   isc, ip6, ip3, ip7
k7                      linen                   1, ip6, ip3, ip7
a5                      poscil                  k3, ip8, ip5
a6                      poscil                  k3, ip8 * .999, ip5
a7                      poscil                  k3, ip8 * 1.001, ip5
a1                      =                       a5 + a6 + a7
a8                      poscil                  k6, ip9, ip5
a9                      poscil                  k6, ip9 * .999, ip5
a10                     poscil                  k6, ip9 * 1.001, ip5
a11                     =                       a8 + a9 + a10
a2                      butterbp                a1, k1, 40
a3                      butterbp                a2, k5, k2 * .8
a4                      balance                 a3, a1
a12                     butterbp                a11, k4, 40
a13                     butterbp                a12, k2, k5 * .8
a14                     balance                 a13, a11
a15                     reverb2                 a4, 5, .3
a16                     reverb2                 a4, 4, .2
a17                     =                       (a15 + a4) * k7
a18                     =                       (a16 + a4) * k7
aleft, aright		Pan			p7, (a17 + a18) * iamplitude
p3, aleft, aright	Declick			.15, p3, .25, aleft, aright
			AssignSend		p1, 0., 0.,  .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr                   13 ; Plain plucked string, Michael Gogins
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500
iattack			=			.002
isustain		=			p3
irelease		=			.05
aenvelope               transeg                 1.0, p3, -3.0, 0.1
aexcite                 poscil                  1.0, 1, gisine
asignal1		wgpluck2 		.1, 1.0, ifrequency,         .15, .2
asignal2		wgpluck2 		.1, 1.0, ifrequency * 1.003, .14, .1
asignal3		wgpluck2 		.1, 1.0, ifrequency * 0.997, .16, .1
apluckout               =                       (asignal1 + asignal2 + asignal3) * aenvelope
aleft, aright		Pan			p7, apluckout * iamplitude
p3, aleft, aright	Declick			iattack, p3, irelease, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr 14                ; Rhodes electric piano model, Perry Cook
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500
iattack			=			.002
isustain		=			p3
irelease		=			.05
iindex                  =                       4
icrossfade              =                       3
ivibedepth              =                       0.2
iviberate               =                       6
ifn1                    =                       gisine
ifn2                    =                       gicosine
ifn3                    =                       gisine
ifn4                    =                       gicookblank
ivibefn                 =                       gisine
asignal                 fmrhode                 iamplitude, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
aleft, aright		Pan			p7, asignal
p3, aleft, aright	Declick			iattack, p3, irelease, aleft, aright
			AssignSend		p1, .2, 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   15 ; Tubular bell model, Perry Cook
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 800
iindex                  =                       1
icrossfade              =                       2
ivibedepth              =                       0.2
iviberate               =                       6
ifn1                    =                       gisine
ifn2                    =                       gicook3
ifn3                    =                       gisine
ifn4                    =                       gisine
ivibefn                 =                       gicosine
asignal                 fmbell                  1.0, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.005, p3, .05, aleft, aright
			AssignSend		p1, 0., 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   16 ; FM moderate index, Michael Gogins
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500
iattack			=			.002
isustain		=			p3
irelease		=			.05
icarrier                =                       1
iratio                  =                       1.25
ifmamplitude            =                       8
index                   =                       5.4
ifrequencyb             =                       ifrequency * 1.003
icarrierb               =                       icarrier * 1.004
aindenv                 expseg                  .000001, iattack, 1, isustain, .125, irelease, .000001
aindex                  =                       aindenv * index * ifmamplitude
aouta                   foscili                 1.0, ifrequency, icarrier, iratio, index, 1
aoutb                   foscili                 1.0, ifrequencyb, icarrierb, iratio, index, 1
			; Plus amplitude correction.
asignal                 =                       (aouta + aoutb) * aindenv
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			iattack, p3, irelease, aleft, aright
			AssignSend		p1, 0., 0., .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   17 ; FM moderate index 2, Michael Gogins
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500
iattack			=			.002
isustain		=			p3
irelease		=			.05
icarrier                =                       1
iratio                  =                       1
ifmamplitude            =                       6
index                   =                       2.5
ifrequencyb             =                       ifrequency * 1.003
icarrierb               =                       icarrier * 1.004
aindenv                 expseg                  .000001, iattack, 1.0, isustain, .0125, irelease, .000001
aindex                  =                       aindenv * index * ifmamplitude - .000001
aouta                   foscili                 1.0, ifrequency, icarrier, iratio, index, 1
aoutb                   foscili                 1.0, ifrequencyb, icarrierb, iratio, index, 1
			; Plus amplitude correction.
afmout                  =                       (aouta + aoutb) * aindenv
aleft, aright		Pan			p7, afmout * iamplitude
p3, aleft, aright	Declick			iattack, p3, irelease, aleft, aright
			AssignSend		p1, 0., 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   18 ; Guitar, Michael Gogins
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 500
iattack			=			.002
isustain		=			p3
irelease		=			.05
asigcomp                pluck                   1.0, 440, 440, 0, 1
asig                    pluck                   1.0, ifrequency, ifrequency, 0, 1
af1                     reson                   asig, 110, 80
af2                     reson                   asig, 220, 100
af3                     reson                   asig, 440, 80
aout                    balance                 0.6 * af1+ af2 + 0.6 * af3 + 0.4 * asig, asigcomp
aexp                    expseg                  1.0, iattack, 2.0, isustain, 1.0, irelease, 1.0
aenv                    =                       aexp - 1.0
asignal                 =                       aout * aenv,
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			iattack, p3, irelease, aleft, aright
			AssignSend		p1, 0., 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   19 ;  Flute, James Kelley
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500
ioctave			=			octcps(ifrequency)
icpsp1                  =                       cpsoct(ioctave - .0002)
icpsp2                  =                       cpsoct(ioctave + .0002)
ip4                     =                       0
ip6			=			iamplitude
			if                      (ip4 == int(ip4 / 2) * 2) goto initslurs
			ihold
initslurs:
iatttm                  =                       0.09
idectm                  =                       0.1
isustm                  =                       p3 - iatttm - idectm
idec                    =                       iamplitude * 1.5
ireinit                 =                       -1
			if                      (ip4 > 1) goto checkafterslur
ilast                   =                       0
checkafterslur:
			if                      (ip4 == 1 || ip4 == 3) goto doneslurs
idec                    =                       0
ireinit                 =                       0
doneslurs:
			if                      (isustm <= 0)   goto simpleenv
kamp                    linseg                  ilast, iatttm, ip6, isustm, ip6, idectm, idec, 0, idec
			goto                    doneenv
simpleenv:
kamp                    linseg                  ilast, p3 / 2,ip6, p3 / 2, idec, 0, idec
doneenv:
ilast                   =                       ip6
			; Some vibrato.
kvrandamp               rand                    .1
kvamp                   =                       (8 + p4) *.06 + kvrandamp
kvrandfreq              rand                    1
kvfreq                  =                       5.5 + kvrandfreq
kvbra                   poscil                  kvamp, kvfreq, 1, ireinit
kfreq1                  =                       icpsp1 + kvbra
kfreq2                  =                       icpsp2 + kvbra
			; Noise for burst at beginning of note.
knseenv                 expon                   ip6 / 4, .2, 1
anoise1                 rand                    knseenv
anoise                  tone                    anoise1, 200
a1                      poscil                  kamp, kfreq1, gikellyflute, ireinit
a2                      poscil                  kamp, kfreq2, gikellyflute, ireinit
asignal                 =                       a1 + a2 + anoise
aleft, aright		Pan			p7, asignal
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, .2, 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   20 ; Delayed plucked string, Michael Gogins
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1200
iattack			=			.002
isustain		=			p3
irelease		=			.05
ihertz                  =                       ifrequency
ioctave			=			octcps(ifrequency)
			; Detuning of strings by 4 cents each way.
idetune                 =                       4.0 / 1200.0
ihertzleft              =                       cpsoct(ioctave + idetune)
ihertzright             =                       cpsoct(ioctave - idetune)
igenleft                =                       gisine
igenright               =                       gicosine
kvibrato                poscil                  1.0 / 120.0, 7.0, 1
kexponential            expseg                  1.0, p3 + iattack, 0.0001, irelease, .0001
kenvelope               =                       (kexponential - 0.0001)
ag                      pluck                   kenvelope, cpsoct(ioctave + kvibrato), ifrequency, igenleft, 1
agleft                  pluck                   kenvelope, ihertzleft,  ifrequency, igenleft, 1
agright                 pluck                   kenvelope, ihertzright, ifrequency, igenright, 1
imsleft                 =                       0.2 * 1000
imsright                =                       0.21 * 1000
adelayleft              vdelay                  ag * kenvelope, imsleft, imsleft + 100
adelayright             vdelay                  ag * kenvelope, imsright, imsright + 100
asignal                 =                       agleft + adelayleft + agright + adelayright
			; Highpass filter to exclude speaker cone excursions.
asignal1                butterhp                asignal, 32.0
asignal2                balance                 asignal1, asignal
aleft, aright		Pan			p7, asignal2 * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   21 ; Melody (Chebyshev / FM / additive), Jon Nelson
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 300
iattack			=			.05
isustain		=			p3
irelease		=			.1
ip6 			= 			gichebychev
			; Pitch.
i1                      =                       ifrequency
k100                    randi                   1,10
k101                    poscil                  1, 5 + k100, gisine
k102                    linseg                  0, .5, 1, p3, 1
k100                    =                       i1 + (k101 * k102)
			; Envelope for driving oscillator.
k1                      linenr                  .5, p3 * .3, p3 * .2, 0.01
k2                      line                    1, p3, .5
k1                      =                       k2 * k1
			; Amplitude envelope.
k10                     expseg                  .0001, iattack, 1.0, isustain, 0.8, irelease, .0001
k10                     =                       (k10 - .0001)
			; Power to partials.
k20                     linseg                  1.485, iattack, 1.5, (isustain + irelease), 1.485
			; a1-3 are for cheby with p6=1-4
a1                      poscil                  k1, k100 - .025, gicook3
			; Tables a1 to fn13, others normalize,
a2                      tablei                  a1, ip6, 1, .5
a3                      balance                 a2, a1
			; Try other waveforms as well.
a4                      foscil                  1, k100 + .04, 1, 2.005, k20, gisine
a5                      poscil                  1, k100, gisine
a6                      =                       ((a3 * .1) + (a4 * .1) + (a5 * .8)) * k10
a7                      comb                    a6, .5, 1 / i1
a8                      =                       (a6 * .9) + (a7 * .1)
asignal        		balance         	a8, a1
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, .2, 0.,  .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   22 ; Tone wheel organ by Mikelson
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2500
iphase			=			p2
ikey                    =                       12 * int(p4 - 6) + 100 * (p4 - 6)
ifqc                    =                       ifrequency
			; The lower tone wheels have increased odd harmonic content.
iwheel1                 =                       ((ikey - 12) > 12 ? gitonewheel1 : gitonewheel2)
iwheel2                 =                       ((ikey +  7) > 12 ? gitonewheel1 : gitonewheel2)
iwheel3                 =                        (ikey       > 12 ? gitonewheel1 : gitonewheel2)
iwheel4                 =                       1
			; Start Dur   Amp   Pitch SubFund Sub3rd Fund 2nd 3rd 4th 5th 6th 8th
			; i1   0    6    200    8.04   8       8     8    8   3   2   1   0   4
asubfund                poscil                  8, .5*ifqc,      iwheel1, iphase/(ikey-12)
asub3rd                 poscil                  8, 1.4983*ifqc,  iwheel2, iphase/(ikey+7)
afund                   poscil                  8, ifqc,         iwheel3, iphase/ikey
a2nd                    poscil                  8, 2*ifqc,       iwheel4, iphase/(ikey+12)
a3rd                    poscil                  3, 2.9966*ifqc,  iwheel4, iphase/(ikey+19)
a4th                    poscil                  2, 4*ifqc,       iwheel4, iphase/(ikey+24)
a5th                    poscil                  1, 5.0397*ifqc,  iwheel4, iphase/(ikey+28)
a6th                    poscil                  0, 5.9932*ifqc,  iwheel4, iphase/(ikey+31)
a8th                    poscil                  4, 8*ifqc,       iwheel4, iphase/(ikey+36)
asignal                 =                       asubfund + asub3rd + afund + a2nd + a3rd + a4th + a5th + a6th + a8th
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.25, p3, .5, aleft, aright
			AssignSend		p1, 0., 0., .2, 1
			SendOut			p1, aleft, aright
			endin

			instr                   23 ; Enhanced FM bell, John ffitch
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
ioct			=			octcps(ifrequency)
idur      		=       		15.0
iamp      		=       		iamplitude
ifenv     		=       		giffitch2                      	; BELL SETTINGS:
ifdyn     		=       		giffitch3                      	; AMP AND INDEX ENV ARE EXPONENTIAL
ifq1      		=       		cpsoct(ioct - 1.) * 5.         	; DECREASING, N1:N2 IS 5:7, imax=10
if1       		=         		giffitch1                       ; DURATION = 15 sec
ifq2      		=         		cpsoct(ioct - 1.) * 7.
if2       		=         		giffitch1
imax      		=         		10
aenv      		poscil    		iamp, 1. / idur, ifenv      	; ENVELOPE
adyn      		poscil    		ifq2 * imax, 1. / idur, ifdyn	; DYNAMIC
anoise    		rand      		50.
amod      		poscil    		adyn + anoise, ifq2, if2   	; MODULATOR
acar      		poscil    		aenv, ifq1 + amod, if1     	; CARRIER
          		timout    		0.5, idur, noisend
knenv     		linseg    		iamp, 0.2, iamp, 0.3, 0
anoise3   		rand      		knenv
anoise4   		butterbp  		anoise3, iamp, 100.
anoise5   		balance   		anoise4, anoise3
noisend:
arvb      		nreverb   		acar, 2, 0.1
asignal      		=         		acar + anoise5 + arvb
aleft, aright		Pan			p7, asignal
p3, aleft, aright	Declick			.003, p3, .5, aleft, aright
			AssignSend		p1, 0., 0., .1, 1.0
			SendOut			p1, aleft, aright
			endin
			

			instr			24 ; STK BandedWG
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 8
asignal 		STKBandedWG 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.006, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin
			
			instr			25 ; STK BeeThree
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 200
asignal 		STKBeeThree 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin
			
			instr			26 ; STK BlowBotl
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 400
asignal 		STKBlowBotl 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			27 ; STK BlowHole
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 500
asignal 		STKBlowHole 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			28 ; STK Bowed
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 50
asignal 		STKBowed 		ifrequency, 1.0, 1, 4, 2, 0, 4, 0, 11, 50
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0.0, 0., .1, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			29 ; STK Brass
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 100
asignal 		STKBrass 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			30 ; STK Clarinet
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 375
asignal 		STKClarinet 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			31; STK Drummer
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 500
asignal 		STKDrummer 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			32 ; STK Flute
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 260
asignal 		STKFlute 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			33 ; STK FMVoices
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 425
asignal 		STKFMVoices 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			34 ; STK HevyMetl
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 180
asignal 		STKHevyMetl 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			35 ; STK Mandolin
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 350
asignal 		STKMandolin 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			36 ; STK ModalBar
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 100
asignal 		STKModalBar 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			37 ; STK Moog
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 375
asignal 		STKMoog 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			38 ; STK PercFlut
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 150
asignal 		STKPercFlut 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			39 ; STK Plucked
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 425
asignal 		STKPlucked 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			40 ; STK Resonate
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 240
asignal 		STKResonate 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			41 ; STK Rhodey
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 225
asignal 		STKRhodey 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			42 ; STK Saxofony
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 550
asignal 		STKSaxofony 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			43 ; STK Shakers
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 100
asignal 		STKShakers 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			44 ; STK Simple
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 200
asignal 		STKSimple 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			45 ; STK Sitar
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 275
asignal 		STKSitar 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			46 ; STK StifKarp
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 400
asignal 		STKStifKarp 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			47 ; STK TubeBell
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 500
asignal 		STKTubeBell 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			48 ; STK VoicForm
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 200
asignal 		STKVoicForm 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			49 ; STK Whistle
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1000
asignal 		STKWhistle 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr			50 ; STK Wurley
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 250
asignal 		STKWurley 		ifrequency, 1.0
aleft, aright		Pan			p7, asignal * iamplitude
p3, aleft, aright	Declick			.003, p3, .05, aleft, aright
			AssignSend		p1, 0., 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin

			instr 			51 ; FluidSynth Steinway
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
			; Use channel assigned in fluidload.
ichannel		=			0
; ioffset			=			((sr / 44100) - 1) * 12
; ikey	 		= 			p4 - ioffset
ikey 			=			p4
ivelocity 		= 			dbamp(iamplitude)
			fluidNote		giFluidsynth, ichannel, ikey, ivelocity
			endin

			instr 			52 ; FluidSynth General MIDI
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
			; Use channel assigned in fluidload.
ichannel		=			1
; ioffset			=			((sr / 44100) - 1) * 12
; ikey	 		= 			p4 - ioffset
ikey 			=			p4
ivelocity 		= 			dbamp(iamplitude)
			fluidNote		giFluidsynth, ichannel, ikey, ivelocity
			endin

			instr 			53 ; FluidSynth Marimba
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
			; Use channel assigned in fluidload.
ichannel		=			2
; ioffset			=			((sr / 44100) - 1) * 12
; ikey	 		= 			p4 - ioffset
ikey 			=			p4
ivelocity 		= 			dbamp(iamplitude)
			fluidNote		giFluidsynth, ichannel, ikey, ivelocity
			endin

			instr 			54 ; FluidSynth Organ
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
			; Use channel assigned in fluidload.
ichannel		=			3
; ioffset			=			((sr / 44100) - 1) * 12
; ikey	 		= 			p4 - ioffset
ikey 			=			p4
ivelocity 		= 			dbamp(iamplitude)
			fluidNote		giFluidsynth, ichannel, ikey, ivelocity
			endin
			
			instr			55 ; Modeled guitar by Jeff Livingston
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			; Original pfields
			; p1     p2   p3    p4    p5    p6      p7      p8       p9        p10         p11     p12      p13
			; in     st   dur   amp   pch   plklen  fbfac	pkupPos	 pluckPos  brightness  vibf    vibd     vibdel
			; i01.2	 0.5  0.75  5000  7.11	.85     0.9975	.0	 .25	   1	       0	0	0
                        pset                    0, 0,3600, 60, 80, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2000
p3, adamping		Damping			.003, p3,.03
ip4			=			iamplitude
; ip5			=			pchcps(ifrequency)
ip6			=			.85
ip7			=			.9975
ip8			=			0
ip9			=			.25
ip10			=			1
ip11			=			0
ip12			=			0
ip13			=			.3333
afwav   		init 			0
abkwav  		init 			0
abkdout 		init 			0
afwdout 		init 			0 
iEstr	 		= 			1 / cpspch(6.04)
ifqc   			= 			ifrequency ; cpspch(ip5)
			print			ifqc
idlt   			= 			1 / ifqc ; note: delay time = 2 x length of string (time to traverse it)
			print 			idlt
ipluck 			= 			.5 * idlt * ip6 * ifqc / cpspch(8.02)
ifbfac 			= 			ip7  			; feedback factor
ibrightness 		= 			ip10 * exp(ip6 * log(2)) / 2 ; (exponentialy scaled) additive noise to add hi freq content
ivibRate 		= 			ip11	;vibrato rate
ivibDepth 		pow 			2, ip12 / 12
ivibDepth 		= 			idlt - 1 / (ivibDepth * ifqc) ; vibrato depth, +,- ivibDepth semitones
ivibStDly 		= 			p13 			; vibrato start delay (secs)
			; termination impedance model
if0 			= 			10000. ; cutoff freq of LPF due to mech. impedance at the nut (2kHz-10kHz)
iA0 			= 			ip7  ; damping parameter of nut impedance
ialpha 			= 			cos(2 * 3.14159265 * if0 * 1 / sr)
ia0 			= 			.3 * iA0 / (2 * (1 - ialpha)) ; FIR LPF model of nut impedance, H(z)=a0+a1z^-1+a0z^-2
ia1 			= 			iA0 - 2 * ia0
; NOTE each filter pass adds a sampling period delay,so subtract 1/sr from tap time to compensate
; determine (in crude fashion) which string is being played
; icurStr = (ifqc > cpspch(6.04) ? 2 : 1)
; icurStr = (ifqc > cpspch(6.09) ? 3 : icurStr)
; icurStr = (ifqc > cpspch(7.02) ? 4 : icurStr)
; icurStr = (ifqc > cpspch(7.07) ? 5 : icurStr)
; icurStr = (ifqc > cpspch(7.11) ? 6 : icurStr)
ipupos 			= 			ip8 * idlt / 2. ; pick up position (in % of low E string length)
ippos      		= 			ip9 * idlt / 2. ; pluck position (in % of low E string length)
isegF 			= 			1 / sr
isegF2 			=     			ipluck
iplkdelF 		= 			(ipluck / 2 > ippos ? 0 : ippos - ipluck / 2)
isegB 			= 			1 / sr
isegB2 			= 			ipluck
iplkdelB 		= 			(ipluck / 2 > idlt / 2 - ippos ? 0 : idlt / 2 - ippos - ipluck / 2)
			; EXCITATION SIGNAL GENERATION
			; the two excitation signals are fed into the fwd delay represent the 1st and 2nd 
			; reflections off of the left boundary, and two accelerations fed into the bkwd delay 
			; represent the the 1st and 2nd reflections off of the right boundary.
			; Likewise for the backward traveling acceleration waves, only they encouter the 
			; terminations in the opposite order.
ipw 			= 			1
ipamp 			= 			ip4 * ipluck ; 4 / ipluck
aenvstrf 		linseg 			0, isegF, - ipamp / 2, isegF2, 0
adel1			delayr 			idlt
aenvstrf1 		deltapi 		iplkdelF ; initial forward traveling wave (pluck to bridge)
aenvstrf2 		deltapi 		iplkdelB + idlt / 2 ; first forward traveling reflection (nut to bridge) 
			delayw 			aenvstrf
			; inject noise for attack time string fret contact, and pre pluck vibrations against pick 
anoiz 			rand			ibrightness
aenvstrf1 		= 			aenvstrf1 + anoiz * aenvstrf1
aenvstrf2 		= 			aenvstrf2 + anoiz * aenvstrf2
			; filter to account for losses along loop path
aenvstrf2		filter2  		aenvstrf2, 3, 0, ia0, ia1, ia0 
			; combine into one signal (flip refl wave's phase)
aenvstrf 		= 			aenvstrf1 - aenvstrf2
			; initial backward excitation wave  
aenvstrb 		linseg 			0, isegB, - ipamp / 2, isegB2, 0  
adel2			delayr 			idlt
aenvstrb1 		deltapi 		iplkdelB ; initial bdwd traveling wave (pluck to nut)
aenvstrb2 		deltapi 		idlt / 2 + iplkdelF ; first forward traveling reflection (nut to bridge) 
			delayw 			aenvstrb
			; initial bdwd traveling wave (pluck to nut)
			; aenvstrb1	delay	aenvstrb,  iplkdelB
			; first bkwd traveling reflection (bridge to nut)
			; aenvstrb2	delay	aenvstrb, idlt / 2 + iplkdelF
			; inject noise
aenvstrb1 		= 			aenvstrb1 + anoiz * aenvstrb1
aenvstrb2 		= 			aenvstrb2 + anoiz * aenvstrb2
			; filter to account for losses along loop path
aenvstrb2		filter2  		aenvstrb2, 3, 0, ia0, ia1, ia0
			; combine into one signal (flip refl wave's phase)
aenvstrb		=			aenvstrb1 - aenvstrb2
			; low pass to band limit initial accel signals to be < 1/2 the sampling freq
ainputf  		tone  			aenvstrf, sr * .9 / 2
ainputb  		tone  			aenvstrb, sr * .9 / 2
			; additional lowpass filtering for pluck shaping
			; Note, it would be more efficient to combine stages into a single filter
ainputf  		tone  			ainputf, sr *.9 / 2
ainputb  		tone  			ainputb, sr *.9 / 2
			; Vibrato generator
avib 			poscil 			ivibDepth, ivibRate, gisine
avibdl			delayr			(ivibStDly * 1.1) + .001
avibrato		deltapi			ivibStDly
			delayw			avib
			; Dual Delay line, 
			; NOTE: delay length longer than needed by a bit so that the output at t=idlt will be interpolated properly        
			; fwd delay line
afd  			delayr 			(idlt + ivibDepth) * 1.1 ; forward traveling wave delay line
afwav  			deltapi 		ipupos ; output tap point for fwd traveling wave
afwdout			deltapi 		idlt - 1 / sr + avibrato ; output at end of fwd delay (left string boundary)
afwdout			filter2  		afwdout, 3, 0, ia0, ia1, ia0  ; lpf/attn due to reflection impedance		
			delayw  		ainputf + afwdout * ifbfac * ifbfac
			; bkwd delay line
abkwd  			delayr 			(idlt + ivibDepth) * 1.1 ;backward trav wave delay line
abkwav  		deltapi 		idlt / 2 - ipupos ; output tap point for bkwd traveling wave
; abkterm		deltapi			idlt / 2 ; output at the left boundary
abkdout			deltapi 		idlt - 1 / sr + avibrato	; output at end of bkwd delay (right string boundary)
abkdout			filter2  		abkdout, 3, 0, ia0, ia1, ia0  	
delayw  		ainputb + abkdout * ifbfac * ifbfac
			; resonant body filter model, from Cuzzucoli and Lombardo
			; IIR filter derived via bilinear transform method
			; the theoretical resonances resulting from circuit model should be:
			; resonance due to the air volume + soundhole = 110Hz (strongest)
			; resonance due to the top plate = 220Hz
			; resonance due to the inclusion of the back plate = 400Hz (weakest)
aresbod 		filter2 		(afwdout + abkdout), 5,4, .000000000005398681501844749, .00000000000001421085471520200, -.00000000001076383426834582, -00000000000001110223024625157, .000000000005392353230604385, -3.990098622573566, 5.974971737738533, -3.979630684599723, .9947612723736902
asignal			=			1500 * (afwav + abkwav + aresbod * .000000000000000000003);
aleft, aright		Pan			p7, asignal * adamping
			AssignSend		p1, .0, 0., 0.2, 1.2
			SendOut			p1, aleft, aright
			endin

instr 			190 ; Fluidsynth output
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ijunk			= 			p1 + p2 + p3 + p4 + p5
ifrequency,iamplitude	NoteOn p4,p5, 			100.
aleft, aright   	fluidOut		giFluidsynth
aleft			= 			iamplitude * aleft
aright			=			iamplitude * aright
			AssignSend		p1, .0, 0., .2, 1.0
			SendOut			p1, aleft, aright
			endin
			
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; B U S S   E F F E C T S 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

			instr                   200 ; Chorus by J. Lato
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			; p4 = delay in milliseconds
			; p5 = divisor of p4
			; Chorus effect, borrowed from http://www.jlpublishing.com/Csound.htm
			; I made some of its parameters accesible trhough score.
a1                      MixerReceive            200, 0
a2                      MixerReceive            200, 1
idlyml                  =                       p4      ;delay in milliseconds
k1                      poscil                  idlyml/p5, 1, 2
ar1l                    vdelay3                 a1, idlyml/5+k1, 900    ;delayed sound 1
ar1r                    vdelay3                 a2, idlyml/5+k1, 900    ;delayed sound 1
k2                      poscil                  idlyml/p5, .995, 2
ar2l                    vdelay3                 a1, idlyml/5+k2, 700    ;delayed sound 2
ar2r                    vdelay3                 a2, idlyml/5+k2, 700    ;delayed sound 2
k3                      poscil                  idlyml/p5, 1.05, 2
ar3l                    vdelay3                 a1, idlyml/5+k3, 700    ;delayed sound 3
ar3r                    vdelay3                 a2, idlyml/5+k3, 700    ;delayed sound 3
k4                      poscil                  idlyml/p5, 1, 2
ar4l                    vdelay3                 a1, idlyml/5+k4, 900    ;delayed sound 4
ar4r                    vdelay3                 a2, idlyml/5+k4, 900    ;delayed sound 4
aoutl                   =                       (a1+ar1l+ar2l+ar3l+ar4l)*.5
aoutr                   =                       (a2+ar1r+ar2r+ar3r+ar4r)*.5
			; To the reverb unit
			MixerSend               aoutl, 200, 210, 0
			MixerSend               aoutr, 200, 210, 1
			; To the output mixer
			MixerSend               aoutl, 200, 220, 0
			MixerSend               aoutr, 200, 220, 1
			endin

			instr                   201 ; Leslie speaker
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			; p4 = Speaker phase offset
			; p5 = Speaker phase separation
			; Speaker phase offset
ioff                    =                       p4
			; Phase separation between right and left
isep                    =                       p5
a1                      MixerReceive            201, 0
a2                      MixerReceive            201, 1
			; Global input from organ
asig                    =                       a1 + a2
			; Distortion effect A lazy "S" curve.  Use table 6 for more distortion.
asig                    =                       asig / 40000
aclip                   tablei                  asig, gitonewheel5, 1, .5
aclip                   =                       aclip * 16000
			; Delay buffer for rotating speaker
aleslie                 delayr                  .02, 1
			; Acceleration
kenv                    linseg                  .8, 1, 8, 2, 8, 1, .8, 2, .8, 1, 8, 1, 8
kenvlow                 linseg                  .7, 2, 7, 1, 7, 2, .7, 1, .7, 2, 7, 1, 7
			; Upper Doppler Effect
koscl                   poscil                  1, kenv, gitonewheel1, ioff
koscr                   poscil                  1, kenv, gitonewheel1, ioff + isep
kdopl                   =                       .01  - koscl * .0002
kdopr                   =                       .012 - koscr * .0002
aleft                   deltapi                 kdopl
aright                  deltapi                 kdopr
			; Lower Effect
koscllow                poscil                  1, kenvlow, gitonewheel1, ioff
koscrlow                poscil                  1, kenvlow, gitonewheel1, ioff + isep
kdopllow                =                       .01  - koscllow * .0003
kdoprlow                =                       .012 - koscrlow * .0003
aleftlow                deltapi                 kdopllow
arightlow               deltapi                 kdoprlow
			delayw                  aclip
			; Filter Effect
			; Divide into three frequency ranges for directional sound.
			; High Pass
alfhi                   butterbp                aleft,   7000, 6000
arfhi                   butterbp                aright,  7000, 6000
			; Band Pass
alfmid                  butterbp                aleft,   3000, 2000
arfmid                  butterbp                aright,  3000, 2000
			; Low Pass
alflow                  butterlp                aleftlow,   1000
arflow                  butterlp                arightlow,  1000
kflohi                  poscil                  1, kenv, gitonewheel3, ioff
kfrohi                  poscil                  1, kenv, gitonewheel3, ioff + isep
kflomid                 poscil                  1, kenv, gitonewheel4, ioff
kfromid                 poscil                  1, kenv, gitonewheel4, ioff + isep
			; Amplitude Effect on Lower Speaker
kalosc                  =                       koscllow * .4 + 1
karosc                  =                       koscrlow * .4 + 1
			; Add all frequency ranges and output the result.
aoutl                   =                       alfhi * kflohi + alfmid * kflomid + alflow * kalosc
aoutr                   =                       arfhi * kfrohi + arfmid * kfromid + arflow * karosc
			; To the reverb unit
			MixerSend               aoutl, 201, 210, 0
			MixerSend               aoutr, 201, 210, 1
			; To the output mixer
			MixerSend               aoutl, 201, 220, 0
			MixerSend               aoutr, 201, 220, 1
			endin

			instr                   210 ; Reverb by Sean Costello / J. Lato
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			; p4 = gain of reverb. Adjust empirically
			; for desired reverb time. .6 gives
			; a good small "live" room sound, .8
			; a small hall, .9 a large hall,
			; .99 an enormous stone cavern.

			; p5 = amount of random pitch modulation
			; for the delay lines. 1 is the "normal"
			; amount, but this may be too high for
			; held pitches such as piano tones.
			; Adjust to taste.

			; p6 = cutoff frequency of lowpass filters
			; in feedback loops of delay lines,
			; in Hz. Lower cutoff frequencies results
			; in a sound with more high-frequency
			; damping.

			; 8 delay line FDN reverb, with feedback matrix based upon 
			; physical modeling scattering junction of 8 lossless waveguides
			; of equal characteristic impedance. Based on Julius O. Smith III, 
			; "A New Approach to Digital Reverberation using Closed Waveguide
			; Networks," Proceedings of the International Computer Music 
			; Conference 1985, p. 47-53 (also available as a seperate
			; publication from CCRMA), as well as some more recent papers by
			; Smith and others.
			; Coded by Sean Costello, October 1999
igain 			= 			p4      
ipitchmod 		= 			p5  
itone 			= 			p6      		
ain1 			MixerReceive 		210, 0
ain2 			MixerReceive 		210, 1
asignal 		= 			(ain1 + ain2) * 0.5
afilt1 			init 			0
afilt2 			init 			0
afilt3 			init 			0
afilt4 			init 			0
afilt5 			init 			0
afilt6 			init 			0
afilt7 			init 			0
afilt8 			init 			0
idel1 			= 			(2473.000/sr)
idel2 			= 			(2767.000/sr)
idel3 			= 			(3217.000/sr)
idel4 			= 			(3557.000/sr)
idel5 			= 			(3907.000/sr)
idel6 			= 			(4127.000/sr)
idel7 			= 			(2143.000/sr)
idel8 			= 			(1933.000/sr)
			; k1-k8 are used to add random pitch modulation to the
			; delay lines. Helps eliminate metallic overtones
			; in the reverb sound.
k1      		randi   		.001, 3.1, .06
k2      		randi   		.0011, 3.5, .9
k3      		randi   		.0017, 1.11, .7
k4      		randi   		.0006, 3.973, .3
k5      		randi   		.001, 2.341, .63
k6      		randi   		.0011, 1.897, .7
k7      		randi   		.0017, 0.891, .9
k8      		randi   		.0006, 3.221, .44
			; apj is used to calculate "resultant junction pressure" for 
			; the scattering junction of 8 lossless waveguides
			; of equal characteristic impedance. If you wish to
			; add more delay lines, simply add them to the following 
			; equation, and replace the .25 by 2/N, where N is the 
			; number of delay lines.
apj 			= 			.25 * (afilt1 + afilt2 + afilt3 + afilt4 + afilt5 + afilt6 + afilt7 + afilt8)
adum1   		delayr  		1
adel1   		deltapi 		idel1 + k1 * ipitchmod
        		delayw  		asignal + apj - afilt1
adum2   		delayr  		1
adel2   		deltapi 		idel2 + k2 * ipitchmod
        		delayw  		asignal + apj - afilt2
adum3   		delayr  		1
adel3   		deltapi 		idel3 + k3 * ipitchmod
        		delayw  		asignal + apj - afilt3
adum4   		delayr  		1
adel4   		deltapi 		idel4 + k4 * ipitchmod
        		delayw  		asignal + apj - afilt4
adum5   		delayr  		1
adel5   		deltapi 		idel5 + k5 * ipitchmod
        		delayw  		asignal + apj - afilt5
adum6   		delayr  		1
adel6   		deltapi 		idel6 + k6 * ipitchmod
          		delayw  		asignal + apj - afilt6
adum7   		delayr  		1
adel7   		deltapi 		idel7 + k7 * ipitchmod
        		delayw  		asignal + apj - afilt7
adum8   		delayr  		1
adel8   		deltapi 		idel8 + k8 * ipitchmod
        		delayw  		asignal + apj - afilt8
			; 1st order lowpass filters in feedback
			; loops of delay lines.
afilt1  		tone    		adel1 * igain, itone
afilt2  		tone    		adel2 * igain, itone
afilt3  		tone    		adel3 * igain, itone
afilt4  		tone    		adel4 * igain, itone
afilt5  		tone    		adel5 * igain, itone
afilt6  		tone    		adel6 * igain, itone
afilt7  		tone    		adel7 * igain, itone
afilt8  		tone    		adel8 * igain, itone
			; The outputs of the delay lines are summed
			; and sent to the stereo outputs. This could
			; easily be modified for a 4 or 8-channel 
			; sound system.
aout1 			= 			(afilt1 + afilt3 + afilt5 + afilt7)
aout2 			= 			(afilt2 + afilt4 + afilt6 + afilt8)
			; To the master output.
			MixerSend 		aout1, 210, 220, 0
			MixerSend 		aout2, 210, 220, 1
			endin

			instr 			220 ; Master output
			;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			; p4 = level
			; p5 = fadein + fadeout
			; Applies a bass enhancement, compression and fadeout
			; to the whole piece, outputs signals, and clears the mixer.
			; Receive audio from the master mixer buss.
a1                      MixerReceive            220, 0
a2                      MixerReceive            220, 1
			; Enhance the bass.
al1                     butterlp                a1, 100
al2                     butterlp                a2, 100
a1                      =                       al1 * 1.5 + a1
a2                      =                       al2 * 1.5 + a2
			; Fade in, fade out.
kenv                    linseg                  0., p5 / 2.0, p4, p3 - p5, p4, p5 / 2.0, 0.
a1                      =                       a1 * kenv
a2                      =                       a2 * kenv
			; Apply compression.
a1                      dam                     a1, 5000, 0.5, 1, 0.2, 0.1
a2                      dam                     a2, 5000, 0.5, 1, 0.2, 0.1
			; Remove DC bias.
a1blocked               dcblock                 a1
a2blocked               dcblock                 a2
			; Output audio.
			outs                    a1blocked, a2blocked
			; Reset the busses for the next kperiod.
			MixerClear
			endin
</CsInstruments>
<CsScore>

; EFFECTS MATRIX

; Chorus to Reverb
;i 1 0 0 200 210 0.1
; Leslie to Reverb
; i 1 0 0 201 210 0.5
; Chorus to Output
;i 1 0 0 200 220 0.2
; Leslie to Output
;i 1 0 0 201 220 0.5
; Reverb to Output
;i 1 0 0 210 220 1.0

; SOUNDFONTS OUTPUT

; Insno Start   Dur     Key 	Amplitude
;i 190 	0       10000   0	80.

; MASTER EFFECT CONTROLS

; Leslie
; Insno Start   Dur     Offset  Separation
; i 201   0     10000   0.05    0.2

; Chorus.
; Insno	Start	Dur	Delay	Divisor of Delay
;i 200   0       10000  10      30

; Reverb.
; Insno	Start	Dur	Level	Feedback	Cutoff
;i 210   0       10000   0.88    0.5  		16000

; Master output.
; Insno	Start	Dur	Fadein	Fadeout
;i 220   0       10000   0.1     0.1

</CsScore>
</CsoundSynthesizer>
