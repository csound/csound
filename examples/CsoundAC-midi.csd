<CsoundSynthesizer>
<CsOptions>
csound -f -h -M0 -d -m3 --midi-key=4 --midi-velocity=5 -odac6 temp.orc temp.sco
</CsOptions>
<CsInstruments>
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; T H E   S I L E N C E   O R C H E S T R A
; Copyright (c) 2006, 2008 by Michael Gogins
; This file is licensed under the GNU Lesser General Public License
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; OBJECTIVES
;
; - Support algorithmic composition in time and pitch
; - Most beautiful sounds
; - Highest precision
; - Lowest noise
; - No clicks
; - MIDI/offline interoperability
; - Gains normalized across instruments, pitches, velocities
; - Modular code
; - READABLE code!
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
;	    implement arbitrary audio transforms)
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

; Define to enable the use of SoundFonts.
#define ENABLE_SOUNDFONTS #1#

; Define to enable the use of VST plugins in general.
#define ENABLE_VST #1#

; Define to enable the use of the Pianoteq VST instrument in particular.
#define ENABLE_PIANOTEQ #1#

sr                      =                       44100
ksmps			        =                       15
nchnls                  =                       2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; A S S I G N   M I D I   C H A N N E L S   T O   I N S T R U M E N T S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                        massign			        0, 42

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; V S T   P L U G I N S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Disabled for Csound installer -- enable if you have the Pianoteq VST plugin.
#ifdef ENABLE_VST

giAzr3                  vstinit                 "D:\\utah\\opt\\Steinberg\\Cubase4\\VSTPlugins\\Synths\\AZR3.dll", 1
                        vstinfo                 giAzr3

#end
                        
#ifdef ENABLE_PIANOTEQ
            
giPianoteq              vstinit                 "D:\\utah\\opt\\Pianoteq\\pianoteq20", 0
                        vstinfo                 giPianoteq

#end
            
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; S O U N D F O N T   A S S I G N M E N T S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Disabled for Csound installer -- enable if you have the SoundFonts.

#ifdef ENABLE_SOUNDFONTS = 1

giFluidsynth		    fluidEngine		        0, 0
giFluidSteinway		    fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\Piano Steinway Grand Model C (21,738KB).sf2",  giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 0, giFluidSteinway, 0, 1

giFluidGM		        fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\63.3mg The Sound Site Album Bank V1.0.SF2", giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 1, giFluidGM, 0, 26

giFluidMarimba		    fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\Marimba Moonman (414KB).SF2", giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 2, giFluidMarimba, 0, 0

giFluidOrgan		    fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\Organ Jeux V1.4 (3,674KB).SF2", giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 3, giFluidOrgan, 0, 40
                        
#end

gi2fqc                  init                    cpspch(7.09)
gi3fqc                  init                    cpspch(10.0)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; F U N C T I O N   T A B L E S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                        ; Waveform for the string-pad
giwave                  ftgen                   1, 0, 65537,    10,     1, .5, .33, 0.25,  .0, 0.1,  .1, 0.1
gisine                  ftgen                   2, 0, 65537,    10,     1
giharpsichord           ftgen                   0, 0, 65537,     7,	    -1, 1024, 1, 1024, -1 ; Kelley harpsichord.
gicosine                ftgen                   0, 0, 65537,    11,     1 ; Cosine wave. Get that noise down on the most widely used table!
giexponentialrise       ftgen                   0, 0, 65537,     5,     .001, 513, 1 ; Exponential rise.
githirteen              ftgen                   0, 0, 65537,     9,     1, .3, 0
giln                    ftgen                   0, 0, 65537,   -12,    20.0 ; Unscaled ln(I(x)) from 0 to 20.0.
gibergeman              ftgen                   0, 0, 65537,    10,     .28, 1, .74, .66, .78, .48, .05, .33, 0.12, .08, .01, .54, 0.19, .08, .05, 0.16, .01, 0.11, .3, .02, 0.2 ; Bergeman f1
gicookblank             ftgen                   0, 0, 65537,    10,     0 ; Blank wavetable for some Cook FM opcodes.
gicook3                 ftgen                   0, 0, 65537,    10,     1, .4, 0.2, 0.1, 0.1, .05
gikellyflute            ftgen                   0, 0, 65537,    10,     1, 0.25, 0.1 ; Kelley flute.
gichebychev             ftgen                   0, 0, 65537,    -7,    -1, 150, 0.1, 110, 0, 252, 0
giffitch1               ftgen                   0, 0, 65537,    10,     1
giffitch2               ftgen                   0, 0, 65537,     5,     1, 1024, .01
giffitch3               ftgen                   0, 0, 65537,     5,     1, 1024, .001
                        ; Rotor Tables
gitonewheel1            ftgen                   0, 0, 65537,    10,     1, .02, .01
gitonewheel2            ftgen                   0, 0, 65537,    10,     1, 0, 0.2, 0, 0.1, 0, .05, 0, .02
                        ; Rotating Speaker Filter Envelopes
gitonewheel3            ftgen                   0, 0, 65537,     7,     0, 110, 0, 18, 1, 18, 0, 110, 0
gitonewheel4            ftgen                   0, 0, 65537,     7,     0, 80, 0.2, 16, 1, 64, 1, 16, 0.2, 80, 0
                        ; Distortion Tables
gitonewheel5            ftgen                   0, 0, 65537,     8,    -.8, 336, -.78,  800, -.7, 5920, .7,  800, .78, 336, .8
gitonewheel6            ftgen                   0, 0, 65537,     8     -.8, 336, -.76, 3000, -.7, 1520, .7, 3000, .76, 336, .8
                        ; Table for Reed Physical Model
gireedtable             ftgen                   0, 0, 256,       7,     1, 80, 1, 156, -1, 40, -1
                        ; Tables for simple granular synthesis
gigrtab                 ftgen                   0, 0, 65537,    10,     1, 0.3, .1 0, .2, .02, 0, .1, .04
giwintab                ftgen                   0, 0, 65537,    10,     1, 0, .5, 0, .33, 0, .25, 0, .2, 0, .167
                        ; Tables for waveshaping drone
giharmonics             ftgen                   0, 0, 65537,    10,     1,   0,   2,   0,   0,   1 
gidistortion            ftgen                   0, 0, 65537,    13,     1,   1,   0,   1,   0,   1    


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; U S E R - D E F I N E D   O P C O D E S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        
                        opcode 			        AssignSend, 0, iiiii
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
insno,ic,il,ir,id 	    xin
inum			        =			            floor(insno)
                        ;print                   inum, ic, il, ir, id
                        MixerSetLevel	 	    inum, 200, ic
                        ;MixerSetLevel	 	    inum, 201, il
                        MixerSetLevel	 	    inum, 210, ir
                        MixerSetLevel	 	    inum, 220, id
                        endop
                        
                        opcode			        NoteOn, ii, iii
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ikey,ivelocity,imeasure xin
inormal			        =			            ampdb(80)
ifrequency 		        = 			            cpsmidinn(ikey)
                        ; Normalize so iamplitude for p5 of 80 == ampdb(80) == 10000.
                        ; This should be the half-amplitude (-6 dB) level in the soundfile.
imidiamplitude 		    =                       ampdb(ivelocity)
iamplitude              =                       imidiamplitude * inormal / imeasure
                        ; print                   ifrequency, inormal, imidiamplitude, imeasure, iamplitude
                        xout			        ifrequency, iamplitude
                        endop
                        
                        opcode			        SendOut, 0, iaa
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
insno, aleft, aright	xin
inum                    =                       floor(insno)
                        MixerSend               aleft, inum, 200, 0
                        MixerSend               aright, inum, 200, 1
                        MixerSend               aleft, inum, 210, 0
                        MixerSend               aright, inum, 210, 1
                        MixerSend               aleft, inum, 220, 0
                        MixerSend               aright, inum, 220, 1
                        ;print                   inum
                        endop

                        opcode			        Pan, aa, ka
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
kpan, asignal		    xin
                        ;  Constant-power pan.
apan                    =                       (kpan / 2.0) + 0.5
aleft, aright           pan2                    asignal, apan
                        xout			        aleft, aright
                        endop

                        opcode			        Declick, iaa, iiiaa
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
iatt,idur,irel,a1,a2	xin
                        if (idur > 0)           then
isustain		        = 			            idur
idur			        =			            iatt + isustain + irel                        
                        else
isustain                =                       100000.0
                        endif                        
aenv			        linsegr			        0.0, iatt, 1.0, isustain, 1.0, irel, 0.0
ab1			            =			            a1 * aenv
ab2			            =			            a2 * aenv
                        xout			        idur, ab1, ab2
                        endop	

                        opcode			        Damping, ia, iii
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
iatt,idur,irel		    xin
                        if (idur > 0)           then
isustain		        = 			            idur
idur			        =			            iatt + isustain + irel                        
                        else
isustain                =                       100000.0
                        endif                        
                        ; Releasing envelope for MIDI performance.
aenv			        linsegr			        0.0, iatt, 1.0, isustain, 1.0, irel, 0.0
                        xout			        idur, aenv
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

                        instr 2                 ; Xanadu instr 1
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000.0
p3,adamping		        Damping			        0.003,  p3, 0.1
ishift			        =           		    8.0 / 1200.0
ipch        		    =           		    ifrequency              	            ; convert parameter 5 to cps.
ioct        		    =           		    octcps(ifrequency)      	            ; convert parameter 5 to oct.
kvib        		    poscil			        1.0 / 120.0, ipch / 50.0, gicosine	    ; vibrato
ag          		    pluck       		    1.0, cpsoct(ioct + kvib),   ipch, 1, 1
agleft      		    pluck       		    1.0, cpsoct(ioct + ishift), ipch, 1, 1
agright     		    pluck       		    1.0, cpsoct(ioct - ishift), ipch, 1, 1
af1         		    expon       		    0.01, 10.0, 1.0             	        ; exponential from 0.1 to 1.0
af2         		    expon       		    0.015, 15., 1.055             	        ; exponential from 1.0 to 0.1
adump       		    delayr      		    2.0                     	            ; set delay line of 2.0 sec
atap1       		    deltap3     		    af1                     	            ; tap delay line with kf1 func.
atap2       		    deltap3     		    af2                     	            ; tap delay line with kf2 func.
ad1         		    deltap3      		    2.0                     	            ; delay 2 sec.
ad2         		    deltap3      		    1.1                     	            ; delay 1.1 sec.
                        delayw      		    ag * adamping                       	; put ag signal into delay line.
aleft 			        = 			            agleft + atap1 + ad1
aright			        =			            agright + atap2 + ad2
aleft, aright		    Pan			            p7, iamplitude * (aleft + aright) * adamping
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 3                 ; Xanadu instr 2
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
p3,adamping		        Damping			        0.01, p3, 0.01
ishift      		    =           		    8.0 / 1200.0
ipch       		        =           		    ifrequency
ioct        		    =           		    octcps(ifrequency) 
kvib        		    poscil       		    1.0 / 80.0, 6.1, gicosine      	        ; vibrato
ag          		    pluck       		    1, cpsoct(ioct + kvib),   ipch, 1, 1
agleft      		    pluck       		    1, cpsoct(ioct + ishift), ipch, 1, 1
agright     		    pluck       		    1, cpsoct(ioct - ishift), ipch, 1, 1
adump       		    delayr      		    0.4                     	            ; set delay line of 0.3 sec
ad1         		    deltap3      		    0.07                     	            ; delay 100 msec.
ad2         		    deltap3      		    0.105                    	            ; delay 200 msec.
                        delayw      		    ag * adamping                  	        ; put ag sign into del line.
aleft			        =			            agleft + ad1
aright			        =			            agright + ad2
aleft, aright		    Pan			            p7, iamplitude * (aleft + aright) * adamping
                        AssignSend		        p1, 0, 0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 4                 ; Xanadu instr 3
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
p3, adamping		    Damping			        0.01, p3, 0.01
ishift      		    =           		    8.0 / 1200.0
ipch        		    =           		    ifrequency
ioct        		    =           		    octcps(ifrequency)
; kadsr       		    linseg      		    0, p3/3, 1.0, p3/3, 1.0, p3/3, 0 	; ADSR envelope
amodi       		    linseg      		    0, p3/3, 5, p3/3, 3, p3/3, 0 		; ADSR envelope for I
ip6			            =			            1.4
ip7			            =			            0.8
amodr       		    linseg      		    ip6, p3, ip7              		; r moves from p6->p7 in p3 sec.
a1          		    =           		    amodi * (amodr - 1 / amodr) / 2
a1ndx       		    =           		    abs(a1 * 2 / 20)            		; a1*2 is normalized from 0-1.
a2          		    =           		    amodi * (amodr + 1 / amodr) / 2
a3          		    tablei      		    a1ndx, giln, 1             		; lookup tbl in f3, normal index
ao1         		    poscil       		    a1, ipch, gicosine             
a4          		    =           		    exp(-0.5 * a3 + ao1)
ao2         		    poscil       		    a2 * ipch, ipch, gicosine        
aoutl       		    poscil       		    1 * a4, ao2 + cpsoct(ioct + ishift), gisine 
aoutr       		    poscil       		    1 * a4, ao2 + cpsoct(ioct - ishift), gisine 
aleft			        =			            aoutl * iamplitude * adamping
aright			        =			            aoutr * iamplitude * adamping
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 5                 ; Tone wheel organ by Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 200000
iphase			        =			            0.0
ikey                    =                       p4 ;12 * int(p4 - 6) + 100 * (p4 - 6)
ifqc                    =                       ifrequency
                        ; The lower tone wheels have increased odd harmonic content.
iwheel1                 =                       ((ikey - 12) > 12 ? gitonewheel1 : gitonewheel2)
iwheel2                 =                       ((ikey +  7) > 12 ? gitonewheel1 : gitonewheel2)
iwheel3                 =                        (ikey       > 12 ? gitonewheel1 : gitonewheel2)
iwheel4                 =                       1
                        ;  Start Dur   Amp   Pitch SubFund Sub3rd Fund 2nd 3rd 4th 5th 6th 8th
                        ;i1   0    6    200    8.04   8       8     8    8   3   2   1   0   4
asubfund                poscil                  8, 0.5 * ifqc,      iwheel1, iphase / (ikey - 12)
asub3rd                 poscil                  8, 1.4983 * ifqc,   iwheel2, iphase / (ikey + 7)
afund                   poscil                  8, ifqc,            iwheel3, iphase /  ikey
a2nd                    poscil                  8, 2 * ifqc,        iwheel4, iphase / (ikey + 12)
a3rd                    poscil                  3, 2.9966 * ifqc,   iwheel4, iphase / (ikey + 19)
a4th                    poscil                  2, 4 * ifqc,        iwheel4, iphase / (ikey + 24)
a5th                    poscil                  1, 5.0397 * ifqc,   iwheel4, iphase / (ikey + 28)
a6th                    poscil                  0, 5.9932 * ifqc,   iwheel4, iphase / (ikey + 31)
a8th                    poscil                  4, 8 * ifqc,        iwheel4, iphase / (ikey + 36)
asignal                 =                       asubfund + asub3rd + afund + a2nd + a3rd + a4th + a5th + a6th + a8th
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.005, p3, 0.3, aleft, aright
                        AssignSend		        p1, 0.0, 0.0,  0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 6                 ; Guitar, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 4000
acomp                   pluck                   iamplitude, 440.0, 440.0, 0, 1
asig                    pluck                   iamplitude, ifrequency, ifrequency / 2.0, 0, 1
aenvelope               transeg                 1.0, 10.0, -5.0, 0.0
af1                     reson                   asig, 110, 80
af2                     reson                   asig, 220, 100
af3                     reson                   asig, 440, 80
asignal                 balance                 0.6 * af1+ af2 + 0.6 * af3 + 0.4 * asig, acomp
aleft, aright		    Pan			            p7, asignal * aenvelope
p3, aleft, aright	    Declick			        0.007, p3, 0.05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.4, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 7                 ; Harpsichord, James Kelley
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 4000
aenvelope               transeg                 1.0, 10.0, -5.0, 0.0
apluck                  pluck                   iamplitude, ifrequency, ifrequency, 0, 1
aharp                   poscil                  aenvelope, ifrequency, giharpsichord
aharp2                  balance                 apluck, aharp
asignal			        =			            apluck + aharp2
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        0.005, p3, 0.3, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 8                 ; Heavy metal model, Perry Cook
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 4000
iindex                  =                       1.1
icrossfade              =                       2
ivibedepth              =                       0.02
iviberate               =                       4.8
ifn1                    =                       gisine
ifn2                    =                       giexponentialrise
ifn3                    =                       githirteen
ifn4                    =                       gisine
ivibefn                 =                       gicosine
adecay                  transeg                 0.0, 0.001, 4, 1.0, 2.0, -4, 0.1, 0.125, -4, 0.0
asignal                 fmmetal                 1.0, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.005, p3, 0.3, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 9                 ; Xing by Andrew Horner
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
                        ; p4 pitch in octave.pch
                        ; original pitch        = A6
                        ; range                 = C6 - C7
                        ; extended range        = F4 - C7
ifrequency,iamplitude	NoteOn                  p4, p5, 2000
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
asignal                 =                       asig * arel * (iamp / inorm) * iamplitude
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        0.005, p3, 0.3, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 10                ; FM modulated left and right detuned chorusing, Thomas Kung
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 20000
iattack 		        = 			            0.25
isustain		        =			            p3
irelease 		        = 			            0.3333333
p3, adamping		    Damping			        iattack, isustain, irelease
ip6                     =                       0.3
ip7                     =                       2.2
ishift      		    =           		    4.0 / 12000.0
ipch       		        =           		    ifrequency
ioct        		    =           		    octcps(ifrequency) 
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
aleft, aright		    Pan			            p7, (aleft + aright) * iamplitude * adamping
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 11                ; String pad, Anthony Kozar
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; String-pad borrowed from the piece "Dorian Gray",
                        ; http://akozar.spymac.net/music/ Modified to fit my needs
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ihz, iamp		        NoteOn                  p4, p5, 15000.0
                        ; Slow attack and release
actrl   		        linseg  		        0,  p3 * 0.5, 1.0,  p3 * 0.5, 0
                        ; Slight chorus effect
afund   		        poscil   		        actrl, ihz,  giwave       	; audio oscillator
acel1   		        poscil   		        actrl, ihz - .1, giwave       	; audio oscilator - flat
acel2   		        poscil   		        actrl, ihz + .1, giwave       	; audio oscillator - sharp
asig    		        =   			        afund + acel1 + acel2
                        ;  Cut-off high frequencies depending on midi-velocity
                        ; (larger velocity implies brighter sound)
;asig 			        butterlp 		        asig, 900 + iamp / 40.
aleft, aright		    Pan			            p7, asig * iamp
p3, aleft, aright	    Declick			        0.25, p3, 0.5, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 12                ; Filtered chorus, Michael Bergeman
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
p3, adamping		    Damping			        0.01, p3, 0.01
ioctave			        =			            octcps(ifrequency)
idb			            = 			            1.5
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
a16                     reverb2                 a4, 4, 0.2
a17                     =                       (a15 + a4) * k7
a18                     =                       (a16 + a4) * k7
aleft, aright		    Pan			            p7, (a17 + a18) * iamplitude * adamping
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 13                ; Plain plucked string, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
iattack			        =			            0.002
isustain		        =			            p3
irelease		        =			            0.05
aenvelope               transeg                 1.0, p3, -3.0, 0.1
aexcite                 poscil                  1.0, 1, gisine
asignal1		        wgpluck2 		        0.1, 1.0, ifrequency,         0.25, 0.22
asignal2		        wgpluck2 		        0.1, 1.0, ifrequency * 1.003, 0.20, 0.223
asignal3		        wgpluck2 		        0.1, 1.0, ifrequency * 0.997, 0.23, 0.224
apluckout               =                       (asignal1 + asignal2 + asignal3) * aenvelope
aleft, aright		    Pan			            p7, apluckout * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 14                ; Rhodes electric piano model, Perry Cook
                         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
iattack			        =			            0.002
isustain		        =			            p3
irelease		        =			            0.05
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
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 15                ; Tubular bell model, Perry Cook
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
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
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.005, p3, 0.05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 16                ; FM moderate index, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
iattack			        =			            0.002
isustain		        =			            p3
irelease		        =			            0.05
icarrier                =                       1
iratio                  =                       1.25
ifmamplitude            =                       8
index                   =                       5.4
ifrequencyb             =                       ifrequency * 1.003
icarrierb               =                       icarrier * 1.004
aindenv                 expseg                  0.000001, iattack, 1, isustain, 0.125, irelease, .000001
aindex                  =                       aindenv * index * ifmamplitude
aouta                   foscili                 1.0, ifrequency, icarrier, iratio, index, 1
aoutb                   foscili                 1.0, ifrequencyb, icarrierb, iratio, index, 1
                        ; Plus amplitude correction.
asignal                 =                       (aouta + aoutb) * aindenv
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 17                ; FM moderate index 2, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 7000
iattack			        =			            0.002
isustain		        =			            p3
irelease		        =			            0.05
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
aleft, aright		    Pan			            p7, afmout * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 19                ;  Flute, James Kelley
                         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
p3, adamping		    Damping			        0.01, p3, 0.01
ioctave			        =			            octcps(ifrequency)
icpsp1                  =                       cpsoct(ioctave - .0002)
icpsp2                  =                       cpsoct(ioctave + .0002)
ip4                     =                       0
ip6			            =			            iamplitude
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
knseenv                 expon                   ip6 / 4, 0.2, 1
anoise1                 rand                    knseenv
anoise                  tone                    anoise1, 200
a1                      poscil                  kamp, kfreq1, gikellyflute, ireinit
a2                      poscil                  kamp, kfreq2, gikellyflute, ireinit
asignal                 =                       a1 + a2 + anoise
aleft, aright		    Pan			            p7, asignal * adamping
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 20                ; Delayed plucked string, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 8000
iattack			        =			            0.002
isustain		        =			            p3
irelease		        =			            0.05
ihertz                  =                       ifrequency
ioctave			        =			            octcps(ifrequency)
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
aleft, aright		    Pan			            p7, asignal2 * iamplitude
p3, aleft, aright	    Declick			        0       .003, p3, 0.05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 21                ; Melody (Chebyshev / FM / additive), Jon Nelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2000
p3, adamping		    Damping			        0.01, p3, 0.01
iattack			        =			            0.05
isustain		        =			            p3
irelease		        =			            0.1
ip6 			        = 			            gichebychev
                        ; Pitch.
i1                      =                       ifrequency
k100                    randi                   1,10
k101                    poscil                  1, 5 + k100, gisine
k102                    linseg                  0, .5, 1, p3, 1
k100                    =                       i1 + (k101 * k102)
                        ; Envelope for driving oscillator.
k1                      linenr                  0.5, p3 * .3, p3 * .2, 0.01
k2                      line                    1, p3, .5
k1                      =                       k2 * k1
                        ; Amplitude envelope.
k10                     expseg                  0.0001, iattack, 1.0, isustain, 0.8, irelease, .0001
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
asignal        		    balance         	    a8, a1
aleft, aright		    Pan			            p7, asignal * iamplitude * adamping
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 22                ; Tone wheel organ by Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 280000
iphase			        =			            p2
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
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.25, p3, .5, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 23                ; Enhanced FM bell, John ffitch
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 15000
ioct			        =			            octcps(ifrequency)
idur      		        =       		        15.0
iamp      		        =       		        iamplitude
ifenv     		        =       		        giffitch2                      	; BELL SETTINGS:
ifdyn     		        =       		        giffitch3                      	; AMP AND INDEX ENV ARE EXPONENTIAL
ifq1      		        =       		        cpsoct(ioct - 1.) * 5.         	; DECREASING, N1:N2 IS 5:7, imax=10
if1       		        =         		        giffitch1                       ; DURATION = 15 sec
ifq2      		        =         		        cpsoct(ioct - 1.) * 7.
if2       		        =         		        giffitch1
imax      		        =         		        10
aenv      		        poscil    		        iamp, 1. / idur, ifenv      	; ENVELOPE
adyn      		        poscil    		        ifq2 * imax, 1. / idur, ifdyn	; DYNAMIC
anoise    		        rand      		        50.
amod      		        poscil    		        adyn + anoise, ifq2, if2   	    ; MODULATOR
acar      		        poscil    		        aenv, ifq1 + amod, if1     	    ; CARRIER
                        timout    		        0.5, idur, noisend
knenv     		        linseg    		        iamp, 0.2, iamp, 0.3, 0
anoise3   		        rand      		        knenv
anoise4   		        butterbp  		        anoise3, iamp, 100.
anoise5   		        balance   		        anoise4, anoise3
noisend:
arvb      		        nreverb   		        acar, 2, 0.1
asignal      		    =         		        acar + anoise5 + arvb
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        0.003, p3, .5, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.1, 1.0
                        SendOut			        p1, aleft, aright
                        endin
            
                        instr 24                ; STK BandedWG
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 200
asignal 		        STKBandedWG 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.006, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin
            
                        instr 25                ; STK BeeThree
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 3800
asignal 		        STKBeeThree 		    ifrequency, 1.0, 1, 1.5, 2, 4.8, 4, 2.1
; ares                  phaser1                 asig, kfreq, kord, kfeedback [, iskip]
aphased                 phaser1                 asignal, 4000, 16, .2, .9
aleft, aright		    Pan			            p7, (asignal + aphased) * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin
            
                        instr 26                ; STK BlowBotl
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 7000
asignal 		        STKBlowBotl 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 27                ; STK BlowHole
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 8000
asignal 		        STKBlowHole 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 28                ; STK Bowed
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 300
asignal 		        STKBowed 		        ifrequency, 1.0, 1, 4, 2, 0, 4, 0, 11, 50
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.1, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 29                ; STK Brass
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1000
asignal 		        STKBrass 		        ifrequency, 2.0, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 30                ; STK Clarinet
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 7000
asignal 		        STKClarinet 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 31                ; STK Drummer
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 8000
asignal 		        STKDrummer 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 32                ; STK Flute
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 5000
asignal 		        STKFlute 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 33                ; STK FMVoices
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 8000
asignal 		        STKFMVoices 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 34                ; STK HevyMetl
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 5000
asignal 		        STKHevyMetl 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 35                ; STK Mandolin
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2700
asignal 		        STKMandolin 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 36                ; STK ModalBar
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1500
asignal 		        STKModalBar 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 37                ; STK Moog
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 4000
asignal 		        STKMoog 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 38                ; STK PercFlut
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2000
asignal 		        STKPercFlut 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 39                ; STK Plucked
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 5000
asignal 		        STKPlucked 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 40                ; STK Resonate
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 3500
asignal 		        STKResonate 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 41                ; STK Rhodey
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 3500
asignal 		        STKRhodey 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 42                ; STK Saxofony
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
asignal 		        STKSaxofony 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 43                ; STK Shakers
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 1000
asignal 		        STKShakers 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 44                ; STK Simple
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 3000
asignal 		        STKSimple 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 45                ; STK Sitar
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2000
asignal 		        STKSitar 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 46                ; STK StifKarp
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 6000
asignal 		        STKStifKarp 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 47                ; STK TubeBell
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 4000
asignal 		        STKTubeBell 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 48                ; STK VoicForm
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2000
asignal 		        STKVoicForm 		    ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 49                ; STK Whistle
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
asignal 		        STKWhistle 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 50                ; STK Wurley
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 3200
asignal 		        STKWurley 		        ifrequency, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

#ifdef ENABLE_SOUNDFONTS

instr 51                ; FluidSynth Steinway
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
                        ; Use channel assigned in fluidload.
ichannel		        =			            0
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
ivelocity 		        = 			            dbamp(iamplitude)
                        fluidNote		        giFluidsynth, ichannel, ikey, ivelocity
                        endin

                        instr 52                ; FluidSynth General MIDI
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
                        ; Use channel assigned in fluidload.
ichannel		        =			            1
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
ivelocity 		        = 			            dbamp(iamplitude)
                        fluidNote		        giFluidsynth, ichannel, ikey, ivelocity
                        endin

                        instr 			        53 ; FluidSynth Marimba
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
                        ; Use channel assigned in fluidload.
ichannel		        =			            2
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
ivelocity 		        = 			            dbamp(iamplitude)
                        fluidNote		    giFluidsynth, ichannel, ikey, ivelocity
                        endin

                        instr 54                ; FluidSynth Organ
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
                        ; Use channel assigned in fluidload.
ichannel		        =			            3
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
ivelocity 		        = 			            dbamp(iamplitude)
                        fluidNote		        giFluidsynth, ichannel, ikey, ivelocity
                        endin

#end

                        instr 55                ; Modeled Guitar, Jeff Livingston
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; The model takes pluck position, and pickup position (in % of string length), and generates
                        ; a pluck excitation signal, representing the string displacement.  The pluck consists 
                        ; of a forward and backward traveling displacement wave, which are recirculated thru two 
                        ; separate delay lines, to simulate the one dimensional string waveguide, with 
                        ; fixed ends.
                        ;
                        ; Losses due to internal friction of the string, and with air, as well as
                        ; losses due to the mechanical impedance of the string terminations are simulated by 
                        ; low pass filtering the signal inside the feedback loops.
                        ; Delay line outputs at the bridge termination are summed and fed into an IIR filter
                        ; modeled to simulate the lowest two vibrational modes (resonances) of the guitar body.
                        ; The theory implies that force due to string displacement, which is equivalent to 
                        ; displacement velocity times bridge mechanical impedance, is the input to the guitar
                        ; body resonator model. Here we have modified the transfer fuction representing the bridge
                        ; mech impedance, to become the string displacement to bridge input force transfer function.
                        ; The output of the resulting filter represents the displacement of the guitar's top plate,
                        ; and sound hole, since thier respective displacement with be propotional to input force.
                        ; (based on a simplified model, viewing the top plate as a force driven spring).
                        ;
                        ; The effects of pluck hardness, and contact with frets during pluck release,
                        ; have been modeled by injecting noise into the initial pluck, proportional to initial 
                        ; string displacement.
                        ;
                        ; Note on pluck shape: Starting with a triangular displacment, I found a decent sounding
                        ; initial pluck shape after some trial and error.  This pluck shape, which is a linear
                        ; ramp, with steep fall off, doesn't necessarily agree with the pluck string models I've 
                        ; studied.  I found that initial pluck shape significantly affects the realism of the 
                        ; sound output, but I the treatment of this topic in musical acoustics literature seems
                        ; rather limited as far as I've encountered.  
                        ;
                        ; Original pfields
                        ; p1     p2   p3    p4    p5    p6      p7      p8       p9        p10         p11    p12   p13
                        ; in     st   dur   amp   pch   plklen  fbfac	pkupPos	 pluckPos  brightness  vibf   vibd  vibdel
                        ; i01.2	 0.5  0.75  5000  7.11	.85     0.9975	.0	    .25	       1	       0	  0	 0
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 20000
p3, adamping		    Damping			        0.003, p3,.03
ip4                     init                    iamplitude
ip6                     init                    0.85
ip7                     init                    0.9975
ip8                     init                    0
ip9                     init                    0.25
ip10                    init                    1.0
ip11                    init                    0.0
ip12                    init                    0.0
ip13                    init                    0.0
afwav                   init                    0
abkwav                  init                    0
abkdout                 init                    0
afwdout                 init                    0 
iEstr	                init                    1.0 / cpspch(6.04)
ifqc                    init                    ifrequency ; cpspch(p5)
                                                ; note:delay time=2x length of string (time to traverse it)
idlt                    init                    1.0 / ifqc		
                        print                   ifrequency, ifqc, idlt
ipluck                  =                       0.5 * idlt * ip6 * ifqc / cpspch(8.02)
ifbfac = ip7  			; feedback factor
                        ; (exponentialy scaled) additive noise to add hi freq content
ibrightness             =                       ip10 * exp(ip6 * log(2)) / 2 
ivibRate                =                       ip11	
ivibDepth               pow                     2, ip12 / 12
                        ; vibrato depth, +,- ivibDepth semitones
ivibDepth               =                       idlt - 1.0 / (ivibDepth * ifqc)	
                        ; vibrato start delay (secs)
ivibStDly               =                       ip13 
                        ; termination impedance model
                        ; cutoff freq of LPF due to mech. impedance at the nut (2kHz-10kHz)
if0                     =                       10000 
                        ; damping parameter of nut impedance
iA0                     =                       ip7  
ialpha                  =                       cos(2 * 3.14159265 * if0 * 1 / sr)
                        ; FIR LPF model of nut impedance,  H(z)=a0+a1z^-1+a0z^-2
ia0                     =                       0.3 * iA0 / (2 * (1 - ialpha))
ia1                     =                       iA0 - 2 * ia0
                        ; NOTE each filter pass adds a sampling period delay,so subtract 1/sr from tap time to compensate
                        ; determine (in crude fashion) which string is being played
                        ; icurStr = (ifqc > cpspch(6.04) ? 2 : 1)
                        ; icurStr = (ifqc > cpspch(6.09) ? 3 : icurStr)
                        ; icurStr = (ifqc > cpspch(7.02) ? 4 : icurStr)
                        ; icurStr = (ifqc > cpspch(7.07) ? 5 : icurStr)
                        ; icurStr = (ifqc > cpspch(7.11) ? 6 : icurStr)
ipupos                  =                       ip8 * idlt / 2 ; pick up position (in % of low E string length)
ippos                   =                       ip9 * idlt / 2 ; pluck position (in % of low E string length)
isegF                   =                       1 / sr
isegF2                  =                       ipluck
iplkdelF                =                       (ipluck / 2 > ippos ? 0 : ippos - ipluck / 2)
isegB                   =                       1 / sr
isegB2                  =                       ipluck
iplkdelB                =                       (ipluck / 2 > idlt / 2 - ippos ? 0 : idlt / 2 - ippos - ipluck / 2)
                        ; EXCITATION SIGNAL GENERATION
                        ; the two excitation signals are fed into the fwd delay represent the 1st and 2nd 
                        ; reflections off of the left boundary, and two accelerations fed into the bkwd delay 
                        ; represent the the 1st and 2nd reflections off of the right boundary.
                        ; Likewise for the backward traveling acceleration waves, only they encouter the 
                        ; terminationsin the opposite order.
ipw                     =                       1
ipamp                   =                       ip4 * ipluck ; 4 / ipluck
aenvstrf                linseg                  0, isegF, -ipamp / 2, isegF2, 0
adel1	                delayr                  idlt
                        ; initial forward traveling wave (pluck to bridge)
aenvstrf1               deltapi                 iplkdelF        
                        ; first forward traveling reflection (nut to bridge) 
aenvstrf2               deltapi                 iplkdelB + idlt / 2 
                        delayw                  aenvstrf
                        ; inject noise for attack time string fret contact, and pre pluck vibrations against pick 
anoiz                   rand	                ibrightness
aenvstrf1               =                       aenvstrf1 + anoiz*aenvstrf1
aenvstrf2               =                       aenvstrf2 + anoiz*aenvstrf2
                        ; filter to account for losses along loop path
aenvstrf2	            filter2                 aenvstrf2, 3, 0, ia0, ia1, ia0 
                        ; combine into one signal (flip refl wave's phase)
aenvstrf                =                       aenvstrf1 - aenvstrf2
                        ; initial backward excitation wave  
aenvstrb                linseg                  0, isegB, - ipamp / 2, isegB2, 0  
adel2	                delayr                  idlt
                        ; initial bdwd traveling wave (pluck to nut)
aenvstrb1               deltapi                 iplkdelB        
                        ; first forward traveling reflection (nut to bridge) 
aenvstrb2               deltapi                 idlt / 2 + iplkdelF 
                        delayw                  aenvstrb
                        ; initial bdwd traveling wave (pluck to nut)
;  aenvstrb1	delay	aenvstrb,  iplkdelB
                        ; first bkwd traveling reflection (bridge to nut)
;  aenvstrb2	delay	aenvstrb, idlt/2+iplkdelF
                        ; inject noise
aenvstrb1               =                       aenvstrb1 + anoiz*aenvstrb1
aenvstrb2               =                       aenvstrb2 + anoiz*aenvstrb2
                        ; filter to account for losses along loop path
aenvstrb2	            filter2                 aenvstrb2, 3, 0, ia0, ia1, ia0
                        ; combine into one signal (flip refl wave's phase)
aenvstrb	            =	                    aenvstrb1 - aenvstrb2
                        ; low pass to band limit initial accel signals to be < 1/2 the sampling freq
ainputf                 tone                    aenvstrf, sr * 0.9 / 2
ainputb                 tone                    aenvstrb, sr * 0.9 / 2
                        ; additional lowpass filtering for pluck shaping\
                        ; Note, it would be more efficient to combine stages into a single filter
ainputf                 tone                    ainputf, sr * 0.9 / 2
ainputb                 tone                    ainputb, sr * 0.9 / 2
                        ; Vibrato generator
avib                    poscil                  ivibDepth, ivibRate, 1
avibdl		            delayr		            (ivibStDly * 1.1) + 0.001
avibrato	            deltapi	                ivibStDly
                        delayw		            avib
                        ; Dual Delay line, 
                        ; NOTE: delay length longer than needed by a bit so that the output at t=idlt will be interpolated properly        
                        ;forward traveling wave delay line
afd  		            delayr                  (idlt + ivibDepth) * 1.1
                        ; output tap point for fwd traveling wave
afwav  	                deltapi                 ipupos    	
                        ; output at end of fwd delay (left string boundary)
afwdout	                deltapi                 idlt - 1 / sr + avibrato	
                        ; lpf/attn due to reflection impedance		
afwdout	                filter2                 afwdout, 3, 0, ia0, ia1, ia0  
  			            delayw                  ainputf + afwdout * ifbfac * ifbfac
                        ; backward trav wave delay line
abkwd  	                delayr                  (idlt + ivibDepth) * 1.1
                        ; output tap point for bkwd traveling wave
abkwav  	            deltapi                 idlt / 2 - ipupos		
                        ; output at the left boundary
; abkterm	deltapi	idlt/2				
                        ; output at end of bkwd delay (right string boundary)
abkdout	                deltapi                 idlt - 1 / sr + avibrato	
abkdout	                filter2                 abkdout, 3, 0, ia0, ia1, ia0  	
                        delayw                  ainputb + abkdout * ifbfac * ifbfac
                        ; resonant body filter model, from Cuzzucoli and Lombardo
                        ; IIR filter derived via bilinear transform method
                        ; the theoretical resonances resulting from circuit model should be:
                        ; resonance due to the air volume + soundhole = 110Hz (strongest)
                        ; resonance due to the top plate = 220Hz
                        ; resonance due to the inclusion of the back plate = 400Hz (weakest)
aresbod                 filter2                 (afwdout + abkdout), 5, 4, 0.000000000005398681501844749, .00000000000001421085471520200, -.00000000001076383426834582, -00000000000001110223024625157, .000000000005392353230604385, -3.990098622573566, 5.974971737738533, -3.979630684599723, .9947612723736902
asig                    =                       (1500 * (afwav + abkwav + aresbod * .000000000000000000003)) * adamping
aleft, aright		    Pan			            p7, asig
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin                        
 
#ifdef ENABLE_PIANOTEQ = 1

                        instr 56                ; Pianoteq
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
ichan                   init                    1.0
                        vstnote                 giPianoteq, ichan, p4, p5, p3
                        endin

#end

                        instr 57                ; Epicycloid or Spirograph curve, Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; This set of parametric equations defines the path traced by
                        ; a point on a circle of radius B rotating outside a circle of
                        ; radius A.
                        ;    p1  p2     p3   p4    p5     p6   p7   p8
                        ;        Start  Dur  Amp   Frqc   A    B    Hole
                        ; i  2   0      6    8000  8.00   10   2    1
                        ; i  2   4      4    .     7.11   5.6  0.4  0.8
                        ; i  2   +      4    .     8.05   2    8.5  0.7
                        ; i  2   .      2    .     8.02   4    5    0.6
                        ; i  2   .      2    .     8.02   5    0.5  1.2                        
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
ifqc                    init                    ifrequency
ifqci                   init                    ifrequency ; gi2fqc
; gi2fqc                  init                    ifqc
ip4                     init                    iamplitude
ia                      init                    5.6 ; p6
ib                      init                    0.4 ; p7
ihole                   init                    0.8 ; p8
iscale                  init                    1 / (ia + 2 * ib)
kampenv                 linseg                  0, .02, ip4 * iscale, p3 - .04, ip4 * iscale, .02, 0
kptchenv                linseg                  ifqci, .2 * p3, ifqc, .8 * p3, ifqc
kvibenv                 linseg                  0, .5, 0, .2, 1, .2, 1
kvibr                   oscili                  20, 8, 1
kfqc                    =                       kptchenv + kvibr * kvibenv
                        ; Sine and Cosine
acos1                   oscili                  ia + ib, kfqc, 1, .25
acos2                   oscili                  ib * ihole, (ia - ib) / ib * kfqc, 1, .25
ax                      =                       acos1 + acos2
asin1                   oscili                  ia + ib, kfqc, 1
asin2                   oscili                  ib, (ia - ib) / ib * kfqc, 1
ay                      =                       asin1 - asin2
aleft                   =                       kampenv * ax
aright                  =                       kampenv * ay
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 58                ; Hypocycloid or Spirograph curve, Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; This set of parametric equations defines the path traced by
                        ; a point on a circle of radius B rotating inside a circle of
                        ; radius A.
                        ;   p1  p2     p3   p4    p5     p6   p7   p8
                        ;       Start  Dur  Amp   Frqc   A    B    Hole
                        ; i 3   16     6    8000  8.00  10    2    1
                        ; i 3   20     4    .     7.11   5.6  0.4  0.8
                        ; i 3   +      4    .     8.05   2    8.5  0.7
                        ; i 3   .      2    .     8.02   4    5    0.6
                        ; i 3   .      2    .     8.02   5    0.5  1.2
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
ifqc                    init                    ifrequency        
ip4                     init                    iamplitude
ifqci                   init                    ifrequency
;ifqci                   init                    gi3fqc
;gi3fqc                  init                    ifqc
ia                      =                       5.6 ; p6
ib                      =                       0.4 ; p7
ihole                   =                       0.8 ; p8
iscale                  =                       (ia < ib ? 1 / ib : 1 / ia)
kampenv                 linseg                  0, .1, ip4 * iscale, p3 - .2, ip4 * iscale, .1, 0
kptchenv                linseg                  ifqci, .2 * p3, ifqc, .8 * p3, ifqc
kvibenv                 linseg                  0, .5, 0, .2, 1, .2, 1
kvibr                   oscili                  20, 8, 1
kfqc                    =                       kptchenv+kvibr*kvibenv
                        ; Sine and Cosine
acos1                   oscili                  ia - ib, kfqc, 1, .25
acos2                   oscili                  ib * ihole, (ia - ib) / ib * kfqc, 1, .25
ax                      =                       acos1 + acos2
asin1                   oscili                  ia-ib, kfqc, 1
asin2                   oscili                  ib, (ia - ib) / ib * kfqc, 1
ay                      =                       asin1 - asin2
aleft                   =                       kampenv * ax
aright                  =                       kampenv * ay
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 59                ; Banchoff Klein Bottle, Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ;   p1  p2     p3   p4    p5     p6   p7
                        ;       Start  Dur  Amp   Frqc   U    V  
                        ; i 4   32     6    6000  6.00   3    2
                        ; i 4   36     4    .     5.11   5.6  0.4
                        ; i 4   +      4    .     6.05   2    8.5
                        ; i 4   .      2    .     6.02   4    5
                        ; i 4   .      2    .     6.02   5    0.5
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 30000
ifqc                    init                    ifrequency
ip4                     init                    iamplitude
iu                      init                    3 ; p6
iv                      init                    2 ; p7
irt2                    init                    sqrt(2)
aampenv                 linseg                  0, 0.02, ip4,  p3 - 0.04, ip4, 0.02, 0
                        ; Cosines
acosu                   oscili                  1, iu * ifqc, 1, .25
acosu2                  oscili                  1, iu * ifqc / 2, 1, .25
acosv                   oscili                  1, iv * ifqc, 1, .25
                        ; Sines
asinu                   oscili                  1, iu * ifqc, 1
asinu2                  oscili                  1, iu * ifqc / 2, 1
asinv                   oscili                  1, iv * ifqc, 1
                        ; Compute X and Y
ax                      =                       acosu * (acosu2 * (irt2 + acosv) + asinu2 * asinv * acosv)
ay                      =                       asinu * (acosu2 * (irt2 + acosv) + asinu2 * asinv * acosv)
                        ; Low frequency rotation in spherical coordinates z, phi, theta.
klfsinth                oscili                  1, 4, 1
klfsinph                oscili                  1, 1, 1
klfcosth                oscili                  1, 4, 1, .25
klfcosph                oscili                  1, 1, 1, .25
aox                     =                       -ax * klfsinth + ay * klfcosth
aoy                     =                       -ax * klfsinth * klfcosph - ay * klfsinth * klfcosph + klfsinph
aleft                   =                       aampenv * aox
aright                  =                       aampenv * aoy
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 60                ; Low-level plucked string, Comajuncosas
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; Low level implementation
                        ; of the classic Karplus-Strong algorithm
                        ; fixed pitches : no vibratos or glissandi !
                        ; implemented by Josep M Comajuncosas / Aug´98
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; Initialised with a wide pulse (buzz)
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; f1 0 32769 10 1; sine wave
                        ; t 0 90
                        ; p1  p2   p3  p4    p5   p6
                        ; i1  0    15  6.04  0.1  1500
                        ; i1  2    15  6.11  0.4  1500
                        ; i1  4    15  7.04  0.8  2500
                        ; i1  6    15  7.09  0.5  1100
                        ; i1  8    15  8.02  0.3  4500
                        ; i1  10   15  8.06  0.2  1300
                        ; e
                        ; i1 0.11 15 6.09 .11  1600
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 2500
ip5                     =                       0.3
ip6                     =                       iamplitude
ipluck                  =                       ip5 ; p5; pluck position ( 0 to 1 )
ifreq                   =                       ifrequency ; cpspch(p4)
idlts                   =                       int(kr / ifreq - 0.5) ; set waveguide length (an integer number of samples)
idlt                    =                       idlts / kr ; convert to seconds
kdlt                    init                    idlts ; counter for string initialisation
irems                   =                       kr / ifreq - idlts + 0.5 ; remaining time in fractions of a sample
                        ; set phase delay induced by the FIR lowpass filter
                        ; and the fractional delay in the waveguide
                        iph = (1 - irems) / (1 + irems) ; allpass filter parameter
                        ; approximation valid at low frequencies relative to sr
awgout                  init                    0
                        if  kdlt < 0  goto continue
                        initialise:
;abuzz                   buzz p6, 1 / idlt, p6 * idlt, 1, ipluck
abuzz                   buzz ip6, 1 / idlt, ip6 * idlt, 1, ipluck
                        ; fill the buffer with a bandlimited pulse
                        ; knh controls bandwidth
                        ; harmonic richness grows with volume
acomb                   delay                   abuzz, ipluck / idlt
apulse                  =                       abuzz - acomb
                        ; implement pluck point as a FIR comb filter
                        continue:
areturn                 delayr                  idlt
ainput                  =                       apulse + areturn
alpf                    filter2                 ainput, 2, 0, 0.5, 0.5
                        ; lowpass filter to simulate energy losses
                        ; could be variable to allow damping control
awgout                  filter2                 alpf, 2, 1, iph, 1, iph
                        ; allpass filter to fine tune the instrument
                        ; should be compensated in the delay line
                        ; for better pitch accuracy
                        delayw                  awgout
awgout                  dcblock                 awgout ; this seems necessary
                        ; ideally should be inside the loop, but then
                        ; the phase delay should be compensated
                        ; for better pitch accuracy
                        ; out                     awgout
aleft, aright		    Pan			            p7, awgout
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
kdlt                    =                       kdlt - 1
anoize                  =                       0 ; supress last impulse when waveguide is loaded
;tricky but easy...
                        endin

                        instr 61                ; Bass Physical Model, Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; p1  p2     p3   p4    p5     p6
                        ;     Start  Dur  Amp   Pitch  PluckDur
                        ; i2  128    4    1400  6.00   0.25
                        ; i2  +      2    1200  6.01   0.25
                        ; i2  .      4    1000  6.05   0.5
                        ; i2  .      2     500  6.04   1
                        ; i2  .      4    1000  6.03   0.5
                        ; i2  .      16   1000  6.00   0.5
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 100000
                        ; Initializations
ifqc                    =                       ifrequency
ip4                     =                       iamplitude
ip6                     =                       0.5
ipluck                  =                       1 / ifqc * ip6
kcount                  init                    0
adline                  init                    0
ablock2                 init                    0
ablock3                 init                    0
afiltr                  init                    0
afeedbk                 init                    0
koutenv                 linseg                  0, .01, 1, p3 - .11 , 1, .1 , 0 ; Output envelope
kfltenv                 linseg                  0, 1.5, 1, 1.5, 0 
                        ; This envelope loads the string with a triangle wave.
kenvstr                 linseg                  0, ipluck / 4, -ip4 / 2, ipluck / 2, ip4 / 2, ipluck / 4, 0, p3 - ipluck, 0
aenvstr                 =                       kenvstr
ainput                  tone                    aenvstr, 200
                        ; DC Blocker
ablock2                 =                       afeedbk - ablock3 + .99 * ablock2
ablock3                 =                       afeedbk
ablock                  =                       ablock2
                        ; Delay line with filtered feedback
adline                  delay                   ablock + ainput, 1 / ifqc - 15 / sr
afiltr                  tone                    adline, 400
                        ; Resonance of the body 
abody1                  reson                   afiltr, 110, 40
abody1                  =                       abody1 / 5000
abody2                  reson                   afiltr, 70, 20
abody2                  =                       abody2 / 50000
afeedbk                 =                       afiltr
aout                    =                       afeedbk
                        ; out                     50 * koutenv * (aout + kfltenv * (abody1 + abody2))
asignal                 =                       50 * koutenv * (aout + kfltenv * (abody1 + abody2))
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 62                ; Perry Cook Slide Flute, Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; p1  p2     p3   p4         p5     p6        p7      p8       p9
                        ;     Start  Dur  Amplitude  Pitch  Pressure  Breath  Feedbk1  Feedbk2
                        ; i3  80     16   6000       8.00   0.9       0.036   0.4      0.4
                        ; i3  +      4    .          8.01   0.95      .       .        .
                        ; i3  .      4    .          8.03   0.97      .       .        .
                        ; i3  .      4    .          8.04   0.98      .       .        .
                        ; i3  .      4    .          8.05   0.99      .       .        .
                        ; i3  .      16   .          9.00   1.0       .       .        .
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
aflute1                 init                    0
ifqc                    =                       ifrequency ; cpspch(p5)
ip4                     =                       iamplitude
ip6                     =                       0.99
ip7                     =                       0.036
ip8                     =                       0.4
ip9                     =                       0.4
ipress                  =                       ip6
ibreath                 =                       ip7
ifeedbk1                =                       ip8
ifeedbk2                =                       ip9
                        ; Flow setup
kenv1                   linseg                  0, .06, 1.1 * ipress, .2, ipress, p3 - .16, ipress, .02, 0 
kenv2                   linseg                  0, .01, 1, p3 - .02, 1, .01, 0
kenvibr                 linseg                  0, .5, 0, .5, 1, p3 - 1, 1  ; Vibrato envelope
                        ; The values must be approximately -1 to 1 or the cubic will blow up.
aflow1                  rand                    kenv1
kvibr                   oscili                  0.02 * kenvibr, 5.3, gisine ; 3
                        ; ibreath can be used to adjust the noise level.
asum1                   =                       ibreath * aflow1 + kenv1 + kvibr
asum2                   =                       asum1 + aflute1 * ifeedbk1
afqc                    =                       1 / ifqc - asum1 / 20000 -9 / sr + ifqc / 12000000
                        ; Embouchure delay should be 1/2 the bore delay
                        ; ax delay asum2, (1/ifqc-10/sr)/2
atemp1                  delayr                  1 / ifqc/2
ax                      deltapi                 afqc / 2 ; - asum1/ifqc/10 + 1/1000
                        delayw                  asum2
apoly                   =                       ax - ax * ax * ax
asum3                   =                       apoly + aflute1 * ifeedbk2
avalue                  tone                    asum3, 2000
                        ; Bore, the bore length determines pitch.  Shorter is higher pitch.
atemp2                  delayr                  1 / ifqc
aflute1                 deltapi                 afqc
                        delayw                  avalue
                        ; out                     avalue * p4 * kenv2
asignal                 =                       avalue * ip4 * kenv2
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        0.003, p3, 0.05, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 63                ; Perry Cook Clarinet, Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; p1   p2     p3    p4       p5          p6     p7         p8          p9
                        ;      Start  Dur   Amp      Pitch       Press  Filter     Embouchure  Reed Table
                        ;                   (20000)  (8.00-9.00) (0-2) (500-1200)  (0-1)      
                        ; i4   32     16    6000     8.00        1.5   1000        0.2         1
                        ; i4    +     4     .        8.01        1.8   1000        0.2         1
                        ; i4    .     2     .        8.03        1.6   1000        0.2         1
                        ; i4    .     2     .        8.04        1.7   1000        0.2         1
                        ; i4    .     2     .        8.05        1.7   1000        0.2         1
                        ; i4    .     2     .        9.03        1.7   1000        0.2         1
                        ; i4    .     4     .        8.00        1.7   1000        0.2         1
                        ; i4    +    16     .        9.00        1.8   1000        0.2         1
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
ip4                     =                       iamplitude
ip6                     =                       1.6
ip7                     =                       1000
ip8                     =                       0.2
ip9                     =                       gireedtable
areedbell               init                    0
ifqc                    =                       ifrequency; cpspch(p5)
ifco                    =                       ip7
ibore                   =                       1 / ifqc - 15 / sr
                        ; I got the envelope from Perry Cooke's Clarinet.
kenv1                   linseg                  0, .005, .55 + .3 * ip6, p3 - .015, .55 + .3 * ip6, .01, 0
kenvibr                 linseg                  0, .1, 0, .9, 1, p3 - 1, 1  ; Vibrato envelope
                        ; Supposedly has something to do with reed stiffness?
kemboff                 =                       ip8
                        ; Breath Pressure
avibr                   oscil                   .1 * kenvibr, 5, gisine ; 3
apressm                 =                       kenv1 + avibr
                        ; Reflection filter from the bell is lowpass.
arefilt                 tone                    areedbell, ifco
                        ; The delay from bell to reed.
abellreed               delay                   arefilt, ibore
                        ; Back pressure and reed table look up.
asum2                   =                       - apressm -.95 * arefilt - kemboff
areedtab                tablei                  asum2 / 4 + .34, ip9, 1, .5
amult1                  =                       asum2 * areedtab
                        ; Forward Pressure
asum1                   =                       apressm + amult1
areedbell               delay                   asum1, ibore
aofilt                  atone                   areedbell, ifco
                        ; out                     aofilt * p4
asignal                 =                       aofilt * ip4
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin
                        
                        instr 64                ; Basic Granular Synthesis, Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; f1  0 65536 1 "hahaha.aif" 0 4 0
                        ; f2  0 1024  7 0 224 1 800 0
                        ; f3  0 8192  7 1 8192 -1
                        ; f4  0 1024  7 0 512 1 512 0
                        ; f5  0 1024 10 1 .3 .1 0 .2 .02 0 .1 .04
                        ; f6  0 1024 10 1 0 .5 0 .33 0 .25 0 .2 0 .167
                        ; a0 14 50
                        ; p1   p2     p3    p4    p5    p6     p7      p8      p9    p10
                        ;      Start  Dur  Amp    Freq  GrTab  WinTab  FqcRng  Dens  Fade
                        ; i1   0.0    6.5  700    9.00  5      4       .210    200   1.8
                        ; i1   3.2    3.5  800    7.08  .      4       .042    100   0.8
                        ; i1   5.1    5.2  600    7.10  .      4       .032    100   0.9
                        ; i1   7.2    6.6  900    8.03  .      4       .021    150   1.6
                        ; i1  21.3    4.5  1000   9.00  .      4       .031    150   1.2
                        ; i1  26.5   13.5  1100   6.09  .      4       .121    150   1.5
                        ; i1  30.7    9.3  900    8.05  .      4       .014    150   2.5
                        ; i1  34.2    8.8  700   10.02  .      4       .14     150   1.6
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 50000
ip4                     =                       iamplitude
ip5                     =                       ifrequency
ip6                     =                       gigrtab
ip7                     =                       giwintab
ip8                     =                       0.033
ip9                     =                       150
ip10                    =                       1.6
idur                    =                       p3
iamp                    =                       iamplitude ; p4
ifqc                    =                       ifrequency ; cpspch(p5)
igrtab                  =                       ip6
iwintab                 =                       ip7
ifrng                   =                       ip8
idens                   =                       ip9
ifade                   =                       ip10
igdur                   =                       0.2
kamp                    linseg                  0, ifade, 1, idur - 2 * ifade, 1, ifade, 0
;                                               Amp   Fqc    Dense  AmpOff PitchOff      GrDur  GrTable   WinTable  MaxGrDur
aoutl                   grain                   ip4,  ifqc,  idens, 100,   ifqc * ifrng, igdur, igrtab,   iwintab,  5
aoutr                   grain                   ip4,  ifqc,  idens, 100,   ifqc * ifrng, igdur, igrtab,   iwintab,  5
;                       outs                    aoutl*kamp,  aoutr*kamp
aleft                   =                       aoutl * kamp
aright                  =                       aoutr * kamp
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin
                        
			            instr 65                ; Chebyshev Waveshaping Drone, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ;   p1      p2      p3          p4              p5         p6           p7        p8
                        ;   insno   onset   duration    fundamental     numerator  denominator  velocity  pan
                        ; What I want here is just intonation C major 7, G major 7, G 7, C major with voice leading.
                        ; i   1       0       60          60              1          1            60       -0.875
                        ; i   1       0      180          60              3          2            60        0.000
                        ; i   1       0       60          60             28         15            60        0.875
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
iattack 		        = 			            0.02
idecay                  =                       0.05
isustain		        =			            p3
irelease 		        = 			            0.25
ihertz                  =                       ifrequency
iamp                    =                       iamplitude
kenvelope               transeg                 0.0, iattack / 2.0, 2.5, iamp / 2.0, iattack / 2.0, -2.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 2.5, iamp / 2.0, idecay / 2.0, -2.5, 0.0
asignal                 poscil3                 kenvelope, ihertz, giharmonics
asignal                 distort                 asignal, 0.4, gidistortion
aleft, aright           reverbsc                asignal, asignal, 0.85, 8000, sr, 0.375 
aleft, aright		    Pan			            p7, aleft + aright
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
			            endin
                        
			            instr 66                ; Reverb Sine, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 20000
iattack 		        = 			            0.02
idecay                  =                       0.03
isustain		        =			            p3
irelease 		        = 			            0.25
kenvelope               transeg                 0.0, iattack, 2.5, iamplitude, isustain, 0.0, iamplitude, idecay, 2.5, 0.0
asignal                 poscil3                 kenvelope, ifrequency, gicosine
aleft, aright           reverbsc                asignal, asignal, 0.90, 10000, sr, 0.775 
aleft, aright		    Pan			            p7, (aleft + aright) * 2.0
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
			            endin
                        
			            instr 67                ; Reverb Sine 2, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 20000
iattack 		        = 			            0.02
idecay                  =                       0.03
isustain		        =			            p3
irelease 		        = 			            0.25
kenvelope               transeg                 0.0, iattack, 2.5, iamplitude, isustain, 0.0, iamplitude, idecay, 2.5, 0.0
asignal                 poscil3                 kenvelope, ifrequency, gicosine
aleft, aright           reverbsc                asignal, asignal, 0.80, 10000, sr, 0.375 
aleft, aright		    Pan			            p7, (aleft + aright) * 2.0
p3, aleft, aright	    Declick			        iattack, isustain, idecay, aleft, aright
                        ; print                   p3, iamplitude, iattack, idecay, isustain
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
			            endin
                        
                        instr 68                ; FM with reverberated modulator, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
ifrequency,iamplitude	NoteOn                  p4, p5, 10000
iattack 		        = 			            0.02
isustain		        =			            p3
irelease 		        = 			            0.25
icarrier                =                       1.0
imodulator              =                       1.3
imodulatorHz            =                       ifrequency * imodulator
index                   =                       2.0
imodulatorAmplitude     =                       imodulatorHz * index
kenvelope               transeg                 0.0, iattack, -9.0, 1.0, isustain, -9.0, 0.5, irelease, -4.0, 0.0
kfmenvelope             transeg                 0.0, iattack, -9.0, 2.0, isustain, -9.0, 0.5, irelease, -4.0, 0.0
                        ; Use poscil to get arate FM.
amodulator              poscil                  imodulatorAmplitude * kfmenvelope, imodulatorHz, gisine  
amodl, amodr            reverbsc                amodulator, amodulator, 0.7, sr * 0.75
asignal                 poscil                  1.0, ifrequency + amodl, gisine  
asignal                 =                       asignal * kenvelope
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin


#ifdef ENABLE_SOUNDFONTS

                        instr 190               ; Fluidsynth output
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ijunk			        = 			            p1 + p2 + p3 + p4 + p5
ifrequency,iamplitude	NoteOn                  p4, p5, 450.0
aleft, aright   	    fluidOut		        giFluidsynth
aleft			        = 			            iamplitude * aleft
aright			        =			            iamplitude * aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

#end

#ifdef ENABLE_PIANOTEQ = 1
                        instr 191               ; Pianoteq output
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ijunk			        = 			            p1 + p2 + p3 + p4 + p5
ifrequency,iamplitude	NoteOn                  p4, p5, 2400.0
ainleft                 init                    0.0
ainright                init                    0.0
aleft, aright           vstaudiog               giPianoteq, ainleft, ainright
aleft			        = 			            0.5 * aleft * iamplitude
aright			        =			            0.5 * aright * iamplitude
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

#end
            
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; B U S S   E F F E C T S 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                        instr 200               ; Chorus by J. Lato
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

                        instr 210               ; Reverb by Sean Costello / J. Lato
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
idelay                  =                       p4      
ipitchmod               =                       p5  
icutoff                 =                       p6              
ainL                    MixerReceive            210, 0
ainR                    MixerReceive            210, 1
aoutL, aoutR            reverbsc                ainL, ainR, idelay, icutoff, sr, ipitchmod, 0
                        ; To the master output.
                        MixerSend               aoutL, 210, 220, 0
                        MixerSend               aoutR, 210, 220, 1
                        endin
                        
                        instr 220               ; Master output
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
                        ; Remove DC bias.
a1blocked               dcblock                 a1
a2blocked               dcblock                 a2
                        ; Apply compression.
a1                      compress                a1, a1, 0, 48, 60, 3, .01, .05, .05
a2                      compress                a2, a2, 0, 48, 60, 3, .01, .05, .05
                        ; Output audio.
                        outs                    a1blocked, a2blocked
                        ; Reset the busses for the next kperiod.
                        MixerClear
                        endin
                        
</CsInstruments>
<CsScore>

; EFFECTS MATRIX

; Chorus to Reverb
i 1 0 0 200 210 0.05
; Chorus to Output
i 1 0 0 200 220 0.05
; Reverb to Output
i 1 0 0 210 220 0.125

; SOUNDFONTS OUTPUT

; Insno     Start   Dur     Key 	Amplitude
i 190 	    0       -1      0	    73.

; PIANOTEQ OUTPUT

; Insno     Start   Dur     Key 	Amplitude
i 191 	    0       -1      0	    1.

; MASTER EFFECT CONTROLS

; Chorus.
; Insno	    Start	Dur	    Delay	Divisor of Delay
i 200       0       -1      10      30

; Reverb.
; Insno	    Start	Dur     Delay	Pitchmod	Cutoff
i 210       0       -1      0.81    0.02  		16000

; Master output.
; Insno	    Start	Dur	Fadein	Fadeout
i 220       0       -1   0.1     0.1

f 0 300






</CsScore>
</CsoundSynthesizer>
