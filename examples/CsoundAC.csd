<CsoundSynthesizer>
<CsOptions>
csound -f -h -M0 -d -m99 --midi-key=4 --midi-velocity=5 -odac6 temp.orc temp.sco
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
; - Offline/MIDI real-time interoperability; 
;   enables developing and balancing instruments using a MIDI keyboard
; - Gains normalized across instruments, pitches, velocities, using 
;   the studio standard (0 dB is full scale, signal should average -6 dBFS)
;   (see NoteOn UDO, below).
; - Modular code
; - READABLE code!
;
; TO DO
;
; Add instruments from Cyclic Bells.
; Clean up 'TODO' commented instruments below.
;
; PFIELDS
;
; All instruments use the following standardized set of pfields:
;
; p1 	Instrument number
; p2    Time of note, in absolute seconds from start of performance
; p3 	Duration of note, in seconds
; p4 	MIDI key (may be fractional)
; p5	MIDI velocity (may be fractional), rescaled (0 = -84 dBFS, 127 = 0 dBFS)
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
ksmps			        =                       16
nchnls                  =                       2
0dbfs                   =                       1.0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; A S S I G N   M I D I   C H A N N E L S   T O   I N S T R U M E N T S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                        massign	                0, 23
                        massign                 1, 23
                        
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

#ifdef ENABLE_SOUNDFONTS

giFluidsynth		    fluidEngine		        0, 0
giFluidSteinway		    fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\Piano Steinway Grand Model C (21,738KB).sf2",  giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 0, giFluidSteinway, 0, 1

giFluidGM		        fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\63.3mg The Sound Site Album Bank V1.0.SF2", giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 1, giFluidGM, 0, 59

giFluidMarimba		    fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\Marimba Moonman (414KB).SF2", giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 2, giFluidMarimba, 0, 0

giFluidOrgan		    fluidLoad		        "\\utah\\home\\mkg\\projects\\music\\__library\\soundfonts\\Organ Jeux V1.4 (3,674KB).SF2", giFluidsynth, 1
                        fluidProgramSelect	    giFluidsynth, 3, giFluidOrgan, 0, 40
                        
#end

gi2fqc                  init                    cpspch(7.09)
gi3fqc                  init                    cpspch(10.0)
giseed                  init                    0.5

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; F U N C T I O N   T A B L E S
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                        ; Waveform for the string-pad
giwave                  ftgen                   1, 0, 65537,    10,     1, .5, .33, 0.25,  .0, 0.1,  .1, 0.1
gisine                  ftgen                   2, 0, 65537,    10,     1
giwtsin                 init                    gisine
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
                        ; Tables for Lee Zakian flute
gif1                    ftgen                   0, 0, 65537,    10,     1 
gif2                    ftgen                   0, 0, 16,       -2,     40, 40, 80, 160, 320, 640, 1280, 2560, 5120, 10240, 10240
gif26                   ftgen                   0, 0, 65537,    -10,    2000, 489, 74, 219, 125, 9, 33, 5, 5 
gif27                   ftgen                   0, 0, 65537,    -10,    2729, 1926, 346, 662, 537, 110, 61, 29, 7 
gif28                   ftgen                   0, 0, 65537,    -10,    2558, 2012, 390, 361, 534, 139, 53, 22, 10, 13, 10 
gif29                   ftgen                   0, 0, 65537,    -10,    12318, 8844, 1841, 1636, 256, 150, 60, 46, 11 
gif30                   ftgen                   0, 0, 65537,    -10,    1229, 16, 34, 57, 32 
gif31                   ftgen                   0, 0, 65537,    -10,    163, 31, 1, 50, 31 
gif32                   ftgen                   0, 0, 65537,    -10,    4128, 883, 354, 79, 59, 23 
gif33                   ftgen                   0, 0, 65537,    -10,    1924, 930, 251, 50, 25, 14
gif34                   ftgen                   0, 0, 65537,    -10,    94, 6, 22, 8 
gif35                   ftgen                   0, 0, 65537,    -10,    2661, 87, 33, 18 
gif36                   ftgen                   0, 0, 65537,    -10,    174, 12
gif37                   ftgen                   0, 0, 65537,    -10,    314, 13

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
                        
                        opcode			        NoteOn, ikii, iii
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; General purpose instrument control UDO.
                        ; Returns the pitch at i-rate, the pitch at k-rate
                        ; (with addition of smoothed MIDI pitch bend, if any),
                        ; decibels full scale scaled from MIDI velocity,
                        ; and the amplitude scaled such that 127 == 0 dBFS.
                        ;
                        ; If an instrument is balanced, then its solo peak 
                        ; amplitude at MIDI velocity 127 should be exactly 
                        ; 0 dBFS. If the instrument is too loud (or too soft) 
                        ; at velocity 127, set imeasuredDBFS to the peak level 
                        ; reported by Csound; e.g. for the following messsage:
                        ;
                        ; rtevent:     T 12.257 TT 12.257 M:    +3.35    +3.60
                        ;    number of samples out of range:      511      552  
                        ;
                        ; set the imeasuredDBFS parameter in the NoteOn call 
                        ; in the instrument to 3.6. This will noprmalize the 
                        ; instrument.                      
ikey,ivelocity,imeasureddBFS xin
                        ; Convert MIDI key number to cycles per second.
iHz 		            = 			            cpsmidinn(ikey)
                        ; Modify with MIDI pitch bend, if any.
kpitchbend              pchbend                 -6.0, +6.0    
kpitchbend              =                       kpitchbend + 6.0
iinitialpb              init                    i(kpitchbend)
                        print                   iinitialpb
                        ; Smooth out the stepping in the MIDI control signal.
ksmoothbend             port                    kpitchbend, 0.125, iinitialpb                        
kKey                    =                       ikey + ksmoothbend
kHz                     =                       cpsmidinn(kKey)
                        ; Scale MIDI velocity to decibels.
ipower			        pow			            ivelocity / 127.0, 2.0
idecibels               =			            20.0 * log10(ipower)
imidiamplitude		    =			            ampdbfs(idecibels)
                        ; Normalize so amplitude at velocity 127 == amplitude at full scale.
inormalFS		        =			            ampdbfs(0)
imeasured127            =                       ampdbfs(imeasureddBFS)
inormal                 =                       inormalFS / imeasured127
inormalizedamplitude    =                       imidiamplitude * inormal
                        print                   ivelocity, idecibels, imidiamplitude, inormal, inormalizedamplitude
                        xout			        iHz, kHz, inormalizedamplitude, idecibels
                        endop
                        
                        opcode			        Modulation, k, j
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; Returns +- one octave of pitch bend in MIDI 
                        ; key numbers.
kpitchbend              init                    0
                        xout                    kpitchbend                  
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
            
                        opcode			        ADSR, ia, iiiiiiii
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; Outputs new p3, arate envelope for
                        ; attack time, attack level, 
                        ; decay time, decay level, 
                        ; sustain time (should usually be p3), sustain level,
                        ; release time, release level, slope exponent. 
                        ; Handles real-time by indefinitely extending 
                        ; sustain time and p3.
iat,ial,idt,idl,ist,irt,irl,islope	xin
ip3                     =                       iat + idt + ist + irt             
aenvelope               transeg                 0.0, iat, islope, ial, idt, islope, idl, ist, islope, irl, irt, islope, 0.0
                        xout			        ip3, aenvelope
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 5.0
p3,adamping		        Damping			        0.003,  p3, 0.1
ishift			        =           		    8.0 / 1200.0
kpch        		    =           		    kHz              	            ; convert parameter 5 to cps.
koct        		    =           		    octcps(kHz)      	            ; convert parameter 5 to oct.
kvib        		    poscil			        1.0 / 120.0, kpch / 50.0, gicosine	    ; vibrato
ag          		    pluck       		    1, cpsoct(koct + kvib),   1000, 1, 1
agleft      		    pluck       		    1, cpsoct(koct + ishift), 1000, 1, 1
agright     		    pluck       		    1, cpsoct(koct - ishift), 1000, 1, 1
ag          		    =                       adamping * ag
agleft                  =                       adamping * agleft
agright                 =                       adamping * agright
af1         		    transeg       		    0.1, 5., -3, 1.0, 300, 0, 1.0        	; exponential from 0.1 to 1.0
af2         		    transeg       		    1.0, 5., -3, 0.1, 300, 0, 0.1           ; exponential from 1.0 to 0.1
adump       		    delayr      		    2.0                     	            ; set delay line of 2.0 sec
atap1       		    deltap3     		    af1                     	            ; tap delay line with kf1 func.
atap2       		    deltap3     		    af2                     	            ; tap delay line with kf2 func.
ad1         		    deltap3      		    2.0                     	            ; delay 2 sec.
ad2         		    deltap3      		    1.1                     	            ; delay 1.1 sec.
                        delayw      		    ag                      	            ; put ag signal into delay line.
aleft 			        = 			            agleft + atap1 + ad1 * adamping
aright			        =			            agright + atap2 + ad2 * adamping
aleft                   =                       iamplitude * aleft * adamping
aright                  =                       iamplitude * aright * adamping
aleft, aright		    Pan			            p7, aleft + aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 3                 ; Xanadu instr 2
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 5
p3,adamping		        Damping			        0.006, p3, 0.06
ishift      		    =           		    8.0 / 1200.0
kpch       		        =           		    kHz
koct        		    =           		    octcps(kHz) 
kvib        		    poscil       		    1/120, kpch/50, gicosine      	        ; vibrato
ag          		    pluck       		    1, cpsoct(koct + kvib),   1000, 1, 1
agleft      		    pluck       		    1, cpsoct(koct + ishift), 1000, 1, 1
agright     		    pluck       		    1, cpsoct(koct - ishift), 1000, 1, 1
adump       		    delayr      		    0.3                     	            ; set delay line of 0.3 sec
ad1         		    deltap3      		    0.1                     	            ; delay 100 msec.
ad2         		    deltap3      		    0.2                    	                ; delay 200 msec.
ag                      =                       adamping * ag
agleft                  =                       adamping * agleft
agright                 =                       adamping * agright
                        delayw      		    ag                  	                ; put ag sign into del line.
aleft			        =			            agleft + ad1
aright			        =			            agright + ad2
aleft, aright		    Pan			            p7, iamplitude * (aleft + aright) * adamping
                        AssignSend		        p1, 0, 0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 4                 ; Xanadu instr 3
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 6.5
ip3                     =                       5.0
p3, adamping		    Damping			        0.01, ip3, 0.01
ishift      		    =           		    8.0 / 1200.0
kpch        		    =           		    kHz
koct        		    =           		    octcps(kpch)
; kadsr       		    linseg      		    0, p3/3, 1.0, p3/3, 1.0, p3/3, 0 	; ADSR envelope
amodi       		    linseg      		    0, ip3/3, 5, ip3/3, 3, ip3/3, 0 		; ADSR envelope for I
ip6			            =			            1.4
ip7			            =			            0.8
amodr       		    linseg      		    ip6, ip3, ip7              		; r moves from p6->p7 in p3 sec.
a1          		    =           		    amodi * (amodr - 1 / amodr) / 2
a1ndx       		    =           		    abs(a1 * 2 / 20)            		; a1*2 is normalized from 0-1.
a2          		    =           		    amodi * (amodr + 1 / amodr) / 2
a3          		    tablei      		    a1ndx, giln, 1             		; lookup tbl in f3, normal index
ao1         		    poscil       		    a1, kpch, gicosine             
a4          		    =           		    exp(-0.5 * a3 + ao1)
ao2         		    poscil       		    a2 * kpch, kpch, gicosine        
aoutl       		    poscil       		    1 * a4, ao2 + cpsoct(koct + ishift), gisine 
aoutr       		    poscil       		    1 * a4, ao2 + cpsoct(koct - ishift), gisine 
aleft			        =			            aoutl * iamplitude * adamping
aright			        =			            aoutr * iamplitude * adamping
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 5                 ; Tone wheel organ by Mikelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 25.0
iphase			        =			            0.0
ikey                    =                       p4 ;12 * int(p4 - 6) + 100 * (p4 - 6)
kfqc                    =                       kHz
                        ; The lower tone wheels have increased odd harmonic content.
iwheel1                 =                       ((ikey - 12) > 12 ? gitonewheel1 : gitonewheel2)
iwheel2                 =                       ((ikey +  7) > 12 ? gitonewheel1 : gitonewheel2)
iwheel3                 =                        (ikey       > 12 ? gitonewheel1 : gitonewheel2)
iwheel4                 =                       1
                        ;  Start Dur   Amp   Pitch SubFund Sub3rd Fund 2nd 3rd 4th 5th 6th 8th
                        ;i1   0    6    200    8.04   8       8     8    8   3   2   1   0   4
asubfund                poscil                  8, 0.5 * kfqc,      iwheel1, iphase / (ikey - 12)
asub3rd                 poscil                  8, 1.4983 * kfqc,   iwheel2, iphase / (ikey + 7)
afund                   poscil                  8, kfqc,            iwheel3, iphase /  ikey
a2nd                    poscil                  8, 2 * kfqc,        iwheel4, iphase / (ikey + 12)
a3rd                    poscil                  3, 2.9966 * kfqc,   iwheel4, iphase / (ikey + 19)
a4th                    poscil                  2, 4 * kfqc,        iwheel4, iphase / (ikey + 24)
a5th                    poscil                  1, 5.0397 * kfqc,   iwheel4, iphase / (ikey + 28)
a6th                    poscil                  0, 5.9932 * kfqc,   iwheel4, iphase / (ikey + 31)
a8th                    poscil                  4, 8 * kfqc,        iwheel4, iphase / (ikey + 36)
asignal                 =                       asubfund + asub3rd + afund + a2nd + a3rd + a4th + a5th + a6th + a8th
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.005, p3, 0.3, aleft, aright
                        AssignSend		        p1, 0.0, 0.0,  0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 6                 ; Guitar, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -5.5
acomp                   pluck                   iamplitude, 440.0, 440.0, 0, 1, .1
iHz                     init                    i(kHz)
iHz2                    =                       iHz / 2.0
asig                    pluck                   iamplitude, kHz, iHz2, 0, 1, .1
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -2.43
aenvelope               transeg                 1.0, 10.0, -5.0, 0.0
apluck                  pluck                   iamplitude, kHz, i(kHz), 0, 1
aharp                   poscil                  aenvelope, kHz, giharpsichord
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -8.5
iindex                  =                       1.1
icrossfade              =                       2
ivibedepth              =                       0.02
iviberate               =                       4.8
ifn1                    =                       gisine
ifn2                    =                       giexponentialrise
ifn3                    =                       githirteen
ifn4                    =                       gisine
ivibefn                 =                       gicosine
iattack                 =                       0.002
idecay                  =                       2.0
isustain                =                       p3
irelease                =                       0.05
adecay                  transeg                 0.0, iattack, -4, 1.0, idecay, -4, 0.1, isustain, -4, 0.1, irelease, -4, 0.0
asignal                 fmmetal                 1.0, kHz, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
aleft, aright		    Pan			            p7, asignal * iamplitude * adecay
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -10.0
isine                   =                       1
iinstrument             =                       p1
istarttime              =                       p2
ioctave                 =                       p4
idur                    =                       p3
kfreq                   =                       kHz
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
awt1                    poscil                  amp1, kfreq, gisine
awt2                    poscil                  amp2, 2.7 * kfreq, gisine
awt3                    poscil                  amp3, 4.95 * kfreq, gisine
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 16.5
iattack 		        = 			            0.25
isustain		        =			            p3
irelease 		        = 			            0.3333333
p3, adamping		    Damping			        iattack, isustain, irelease
ip6                     =                       0.3
ip7                     =                       2.2
ishift      		    =           		    4.0 / 12000.0
kpch       		        =           		    kHz
koct        		    =           		    octcps(kHz) 
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
ao1                     poscil                  a1, kpch, gicosine
a4                      =                       exp(-0.5 * a3 + ao1)
                        ; Cosine
ao2                     poscil                  a2 * kpch, kpch, gicosine
                        ; Final output left
aleft                   poscil                  a4, ao2 + cpsoct(koct + ishift), gisine
                        ; Final output right
aright                  poscil                  a4, ao2 + cpsoct(koct - ishift), gisine
aleft, aright		    Pan			            p7, (aleft + aright) * iamplitude * adamping
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 11                ; String pad, Anthony Kozar
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        ; String-pad borrowed from the piece "Dorian Gray",
                        ; http://akozar.spymac.net/music/ Modified to fit my needs
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 3.0
                        ; Slow attack and release
iattack                 =                       2.0
idecay                  =                       2.0
isustain                =                       p3
irelease                =                       0.06
actrl   		        transeg  		        0, iattack, -5, 1.0, idecay, -5, 0.1, isustain, -5, 0.1, irelease, -5, 0
                        ; Slight chorus effect
afund   		        poscil   		        actrl, kHz,  giwave       	; audio oscillator
acel1   		        poscil   		        actrl, kHz - .1, giwave       	; audio oscilator - flat
acel2   		        poscil   		        actrl, kHz + .1, giwave       	; audio oscillator - sharp
asig    		        =   			        afund + acel1 + acel2
                        ;  Cut-off high frequencies depending on midi-velocity
                        ; (larger velocity implies brighter sound)
;asig 			        butterlp 		        asig, 900 + iamp / 40.
aleft, aright		    Pan			            p7, asig * iamplitude
p3, aleft, aright	    Declick			        0.25, p3, 0.5, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 12                ; Filtered chorus, Michael Bergeman
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 71
p3, adamping		    Damping			        0.01, p3, 0.01
koctave			        =			            octcps(kHz)
idb			            = 			            1.5
ip5                     =                       gibergeman
ip3                     =                       5.0
ip6                     =                       0.9
ip7                     =                       1.4
kp8                     =                       cpsoct(koctave - .01)
kp9                     =                       cpsoct(koctave + .01)
isc                     =                       idb * .333
k1                      linseg                  40, ip3, 800, p3, 800, 0.06, 0.0
k2                      linseg                  440, ip3, 220, p3, 220, 0.06, 0.0
k3                      linseg                  0.0, ip6, 800, ip7, 200.0, p3, 200, 0.06, 0.0
k4                      linseg                  800, ip3, 40, p3, 40, 0.06, 0.0
k5                      linseg                  220, ip3, 440, p3, 440, 0.06, 0.0
k6                      linseg                  isc, ip6, p3, ip7, p3, 0.06, 0.0
k7                      linseg                  0.0, ip6, 1, ip7, .3, p3, .1, 0.06, 0.0
a5                      poscil                  k3, kp8, ip5
a6                      poscil                  k3, kp8 * 0.999, ip5
a7                      poscil                  k3, kp8 * 1.001, ip5
a1                      =                       a5 + a6 + a7
a8                      poscil                  k6, kp9, ip5
a9                      poscil                  k6, kp9 * 0.999, ip5
a10                     poscil                  k6, kp9 * 1.001, ip5
a11                     =                       a8 + a9 + a10
a2                      butterbp                a1, k1, 40
a3                      butterbp                a2, k5, k2 * 0.8
a4                      balance                 a3, a1
a12                     butterbp                a11, k4, 40
a13                     butterbp                a12, k2, k5 * 0.8
a14                     balance                 a13, a11
a15                     reverb2                 a4, 5, 0.3
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 5
iattack			        =			            0.002
isustain		        =			            p3
irelease		        =			            0.05
aenvelope               transeg                 1.0, p3, -3.0, 0.1
aexcite                 poscil                  1.0, 1, gisine
asignal1		        wgpluck2 		        0.1, 1.0, iHz,         0.25, 0.22
asignal2		        wgpluck2 		        0.1, 1.0, iHz * 1.003, 0.20, 0.223
asignal3		        wgpluck2 		        0.1, 1.0, iHz * 0.997, 0.23, 0.224
apluckout               =                       (asignal1 + asignal2 + asignal3) * aenvelope
aleft, aright		    Pan			            p7, apluckout * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 14                ; Rhodes electric piano model, Perry Cook
                         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 1
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
asignal                 fmrhode                 iamplitude, kHz, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
aleft, aright		    Pan			            p7, asignal
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 15                ; Tubular bell model, Perry Cook
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -2
iindex                  =                       1
icrossfade              =                       2
ivibedepth              =                       0.2
iviberate               =                       6
ifn1                    =                       gisine
ifn2                    =                       gicook3
ifn3                    =                       gisine
ifn4                    =                       gisine
ivibefn                 =                       gicosine
asignal                 fmbell                  1.0, kHz, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.005, p3, 0.05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 16                ; FM moderate index, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 3.5
iattack			        =			            0.002
isustain		        =			            p3
idecay				    =				        1.5
irelease		        =			            0.05
icarrier                =                       1
imodulator              =                       1
ifmamplitude            =                       8
index                   =                       5.4
ifrequencyb             =                       iHz * 1.003
icarrierb               =                       icarrier * 1.004
aindenv                 transeg                 0.0, iattack, -7.0, 1.0, idecay, -7.0, 0.025, isustain, 0.0, 0.025, irelease, -7.0, 0.0
aindex                  =                       aindenv * index * ifmamplitude
aouta                   foscili                 1.0, iHz, icarrier, imodulator, index, 1
aoutb                   foscili                 1.0, ifrequencyb, icarrierb, imodulator, index, 1
                        ; Plus amplitude correction.
afmout                  =                       (aouta + aoutb) * aindenv
aleft, aright		    Pan			            p7, afmout * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 17                ; FM moderate index 2, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 3.7
iattack			        =			            0.002
isustain		        =			            p3
idecay				    =				        1.5
irelease		        =			            0.05
icarrier                =                       1
imodulator              =                       3
ifmamplitude            =                       8
index                   =                       5.5
ifrequencyb             =                       iHz * 1.003
icarrierb               =                       icarrier * 1.004
aindenv                 transeg                 0.0, iattack, -7.0, 1.0, idecay, -7.0, 0.025, isustain, 0.0, 0.025, irelease, -7.0, 0.0
aindex                  =                       aindenv * index * ifmamplitude
aouta                   foscili                 1.0, iHz, icarrier, imodulator, index, 1
aoutb                   foscili                 1.0, ifrequencyb, icarrierb, imodulator, index, 1
                        ; Plus amplitude correction.
afmout                  =                       (aouta + aoutb) * aindenv
aleft, aright		    Pan			            p7, afmout * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 18                ; FM moderate index 3, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 3.7
iattack			        =			            0.002
isustain		        =			            p3
idecay				    =				        1.5
irelease		        =			            0.05
icarrier                =                       1
imodulator              =                       1.5
ifmamplitude            =                       2
index                   =                       0.5
ifrequencyb             =                       iHz * 1.003
icarrierb               =                       icarrier * 1.004
aindenv                 transeg                 0.0, iattack, -7.0, 1.0, idecay, -7.0, 0.025, isustain, 0.0, 0.025, irelease, -7.0, 0.0
aindex                  =                       aindenv * index * ifmamplitude
; ares                  foscili                 xamp, kcps, xcar, xmod, kndx, ifn [, iphs]
aouta                   foscili                 1.0, iHz, icarrier, imodulator, index, 1
aoutb                   foscili                 1.0, ifrequencyb, icarrierb, imodulator, index, 1
                        ; Plus amplitude correction.
afmout                  =                       (aouta + aoutb) * aindenv
aleft, aright		    Pan			            p7, afmout * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 19                ; Flute, Lee Zakian
                         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -1
p3, adamping		    Damping			        0.01, p3, 0.01
ip3                     =                       (p3 < 3.0 ? p3 : 3.0)
; parameters
                        ; p4    overall amplitude scaling factor
ip4                     init                    iamplitude
                        ; p5    pitch in Hertz (normal pitch range: C4-C7)
ip5                     init                    iHz
                        ; p6    percent vibrato depth, recommended values in range [-1., +1.]
ip6                     init                    1
                        ;        0.0    -> no vibrato
                        ;       +1.     -> 1% vibrato depth, where vibrato rate increases slightly
                        ;       -1.     -> 1% vibrato depth, where vibrato rate decreases slightly
                        ; p7    attack time in seconds 
                        ;       recommended value:  .12 for slurred notes, .06 for tongued notes 
                        ;                            (.03 for short notes)
ip7                     init                    .08
                        ; p8    decay time in seconds 
                        ;       recommended value:  .1 (.05 for short notes)
ip8                     init                    .08
                        ; p9    overall brightness / filter cutoff factor 
                        ;       1 -> least bright / minimum filter cutoff frequency (40 Hz)
                        ;       9 -> brightest / maximum filter cutoff frequency (10,240Hz)
ip9                     init                    5

                        ; initial variables
iampscale               =                       ip4                              ; overall amplitude scaling factor
ifreq                   =                       ip5                              ; pitch in Hertz
ivibdepth               =                       abs(ip6*ifreq/100.0)             ; vibrato depth relative to fundamental frequency
iattack                 =                       ip7 * (1.1 - .2*giseed)          ; attack time with up to +-10% random deviation
giseed                  =                       frac(giseed*105.947)             ; reset giseed
idecay                  =                       ip8 * (1.1 - .2*giseed)          ; decay time with up to +-10% random deviation
giseed                  =                       frac(giseed*105.947)
ifiltcut                tablei                  ip9, gif2                            ; lowpass filter cutoff frequency

iattack                 =                       (iattack < 6/kr ? 6/kr : iattack)               ; minimal attack length
idecay                  =                       (idecay < 6/kr ? 6/kr : idecay)                 ; minimal decay length
isustain                =                       p3 - iattack - idecay
p3                      =                       (isustain < 5/kr ? iattack+idecay+5/kr : p3)    ; minimal sustain length
isustain                =                       (isustain < 5/kr ? 5/kr : isustain)                     
iatt                    =                       iattack/6
isus                    =                       isustain/4
idec                    =                       idecay/6
iphase                  =                       giseed                          ; use same phase for all wavetables
giseed                  =                       frac(giseed*105.947)

                        ; vibrato block
; kvibdepth               linseg                  .1, .8*p3, 1, .2*p3, .7
kvibdepth               linseg                  .1, .8*ip3, 1, isustain, 1, .2*ip3, .7
kvibdepth               =                       kvibdepth* ivibdepth            ; vibrato depth
kvibdepthr              randi                   .1*kvibdepth, 5, giseed         ; up to 10% vibrato depth variation
giseed                  =                       frac(giseed*105.947)
kvibdepth               =                       kvibdepth + kvibdepthr
ivibr1                  =                       giseed                          ; vibrato rate
giseed                  =                       frac(giseed*105.947)
ivibr2                  =                       giseed
giseed                  =                       frac(giseed*105.947)

if                      ip6 < 0 goto            vibrato1
kvibrate                linseg                  2.5+ivibr1, p3, 4.5+ivibr2      ; if p6 positive vibrato gets faster
                        goto                    vibrato2
vibrato1:
ivibr3                  =                       giseed
giseed                  =                       frac(giseed*105.947)
kvibrate                linseg                  3.5+ivibr1, .1, 4.5+ivibr2, p3-.1, 2.5+ivibr3   ; if p6 negative vibrato gets slower
vibrato2:
kvibrater               randi                   .1*kvibrate, 5, giseed          ; up to 10% vibrato rate variation
giseed                  =                       frac(giseed*105.947)
kvibrate                =                       kvibrate + kvibrater
kvib                    oscili                  kvibdepth, kvibrate, giwtsin

ifdev1                  =                       -.03 * giseed                           ; frequency deviation
giseed                  =                       frac(giseed*105.947)
ifdev2                  =                       .003 * giseed
giseed                  =                       frac(giseed*105.947)
ifdev3                  =                       -.0015 * giseed
giseed                  =                       frac(giseed*105.947)
ifdev4                  =                       .012 * giseed
giseed                  =                       frac(giseed*105.947)
kfreqr                  linseg                  ifdev1, iattack, ifdev2, isustain, ifdev3, idecay, ifdev4
kfreq                   =                       kHz * (1 + kfreqr) + kvib

if                      ifreq <  427.28 goto    range1                          ; (cpspch(8.08) + cpspch(8.09))/2
if                      ifreq <  608.22 goto    range2                          ; (cpspch(9.02) + cpspch(9.03))/2
if                      ifreq <  1013.7 goto    range3                          ; (cpspch(9.11) + cpspch(10.00))/2
                        goto                    range4
                        ; wavetable amplitude envelopes
range1:                 ; for low range tones
kamp1                   linseg                  0, iatt, 0.002, iatt, 0.045, iatt, 0.146, iatt,  \
                                                0.272, iatt, 0.072, iatt, 0.043, isus, 0.230, isus, 0.000, isus, \
                                                0.118, isus, 0.923, idec, 1.191, idec, 0.794, idec, 0.418, idec, \
                                                0.172, idec, 0.053, idec, 0
kamp2                   linseg                  0, iatt, 0.009, iatt, 0.022, iatt, -0.049, iatt,  \
                                                -0.120, iatt, 0.297, iatt, 1.890, isus, 1.543, isus, 0.000, isus, \ 
                                                0.546, isus, 0.690, idec, -0.318, idec, -0.326, idec, -0.116, idec, \ 
                                                -0.035, idec, -0.020, idec, 0
kamp3                   linseg                  0, iatt, 0.005, iatt, -0.026, iatt, 0.023, iatt,    \
                        0.133, iatt, 0.060, iatt, -1.245, isus, -0.760, isus, 1.000, isus,  \
                        0.360, isus, -0.526, idec, 0.165, idec, 0.184, idec, 0.060, idec,   \
                        0.010, idec, 0.013, idec, 0
iwt1                    =                       gif26                                      ; wavetable numbers
iwt2                    =                       gif27
iwt3                    =                       gif28
inorm                   =                       3949
                        goto                    end

range2:                 ; for low mid-range tones 

kamp1                   linseg                  0, iatt, 0.000, iatt, -0.005, iatt, 0.000, iatt, \
                                                0.030, iatt, 0.198, iatt, 0.664, isus, 1.451, isus, 1.782, isus, \ 
                                                1.316, isus, 0.817, idec, 0.284, idec, 0.171, idec, 0.082, idec, \ 
                                                0.037, idec, 0.012, idec, 0
kamp2                   linseg                  0, iatt, 0.000, iatt, 0.320, iatt, 0.882, iatt,      \
                                                1.863, iatt, 4.175, iatt, 4.355, isus, -5.329, isus, -8.303, isus,   \
                                                -1.480, isus, -0.472, idec, 1.819, idec, -0.135, idec, -0.082, idec, \ 
                                                -0.170, idec, -0.065, idec, 0
kamp3                   linseg                  0, iatt, 1.000, iatt, 0.520, iatt, -0.303, iatt,     \
                                                0.059, iatt, -4.103, iatt, -6.784, isus, 7.006, isus, 11, isus,      \
                                                12.495, isus, -0.562, idec, -4.946, idec, -0.587, idec, 0.440, idec, \ 
                                                0.174, idec, -0.027, idec, 0
iwt1                    =                       gif29
iwt2                    =                       gif30
iwt3                    =                       gif31
inorm                   =                       27668.2
                        goto                    end

range3:                 ; for high mid-range tones 

kamp1                   linseg                  0, iatt, 0.005, iatt, 0.000, iatt, -0.082, iatt,      \
                                                0.36, iatt, 0.581, iatt, 0.416, isus, 1.073, isus, 0.000, isus,       \
                                                0.356, isus, .86, idec, 0.532, idec, 0.162, idec, 0.076, idec, 0.064, \ 
                                                idec, 0.031, idec, 0
kamp2                   linseg                  0, iatt, -0.005, iatt, 0.000, iatt, 0.205, iatt,      \
                                                -0.284, iatt, -0.208, iatt, 0.326, isus, -0.401, isus, 1.540, isus,   \
                                                0.589, isus, -0.486, idec, -0.016, idec, 0.141, idec, 0.105, idec,    \
                                                -0.003, idec, -0.023, idec, 0
kamp3                   linseg                  0, iatt, 0.722, iatt, 1.500, iatt, 3.697, iatt,       \
                                                0.080, iatt, -2.327, iatt, -0.684, isus, -2.638, isus, 0.000, isus,   \
                                                1.347, isus, 0.485, idec, -0.419, idec, -.700, idec, -0.278, idec,    \
                                                0.167, idec, -0.059, idec, 0
iwt1                    =                       gif32
iwt2                    =                       gif33
iwt3                    =                       gif34
inorm                   =                       3775
                        goto                    end

range4:                                                 ; for high range tones 

kamp1                   linseg                  0, iatt, 0.000, iatt, 0.000, iatt, 0.211, iatt,         \
                                                0.526, iatt, 0.989, iatt, 1.216, isus, 1.727, isus, 1.881, isus,        \
                                                1.462, isus, 1.28, idec, 0.75, idec, 0.34, idec, 0.154, idec, 0.122,    \
                                                idec, 0.028, idec, 0
kamp2                   linseg                  0, iatt, 0.500, iatt, 0.000, iatt, 0.181, iatt,         \
                                                0.859, iatt, -0.205, iatt, -0.430, isus, -0.725, isus, -0.544, isus,    \
                                                -0.436, isus, -0.109, idec, -0.03, idec, -0.022, idec, -0.046, idec,    \
                                                -0.071, idec, -0.019, idec, 0
kamp3                   linseg                  0, iatt, 0.000, iatt, 1.000, iatt, 0.426, iatt,         \
                                                0.222, iatt, 0.175, iatt, -0.153, isus, 0.355, isus, 0.175, isus,       \
                                                0.16, isus, -0.246, idec, -0.045, idec, -0.072, idec, 0.057, idec,      \
                                                -0.024, idec, 0.002, idec, 0
iwt1                    =                       gif35
iwt2                    =                       gif36
iwt3                    =                       gif37
inorm                   =                       4909.05
                        goto                    end

end:
kampr1                  randi                   .02*kamp1, 10, giseed                   ; up to 2% wavetable amplitude variation
giseed                  =                       frac(giseed*105.947)
kamp1                   =                       kamp1 + kampr1
kampr2                  randi                   .02*kamp2, 10, giseed                   ; up to 2% wavetable amplitude variation
giseed                  =                       frac(giseed*105.947)
kamp2                   =                       kamp2 + kampr2
kampr3                  randi                   .02*kamp3, 10, giseed                   ; up to 2% wavetable amplitude variation
giseed                  =                       frac(giseed*105.947)
kamp3                   =                       kamp3 + kampr3

awt1                    poscil                  kamp1, kfreq, iwt1, iphase              ; wavetable lookup
awt2                    poscil                  kamp2, kfreq, iwt2, iphase
awt3                    poscil                  kamp3, kfreq, iwt3, iphase
asig                    =                       awt1 + awt2 + awt3
asig                    =                       asig*(iampscale/inorm)
kcut                    linseg                  0, iattack, ifiltcut, isustain, ifiltcut, idecay, 0     ; lowpass filter for brightness control
afilt                   tone                    asig, kcut
asig                    balance                 afilt, asig
; garev                 =                       garev + asig
;                       outs                    asig, asig
aleft, aright		    Pan			            p7, (asig + asig) * adamping
                        AssignSend		        p1, 0.2, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 20                ; Delayed plucked string, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -1
iattack			        =			            0.006
idecay				    =				        1.5
isustain		        =			            p3
irelease		        =			            0.05
ihertz                  =                       iHz
ioctave			        =			            octcps(iHz)
                        ; Detuning of strings by 4 cents each way.
idetune                 =                       4.0 / 1200.0
ihertzleft              =                       cpsoct(ioctave + idetune)
ihertzright             =                       cpsoct(ioctave - idetune)
igenleft                =                       gisine
igenright               =                       gicosine
kvibrato                poscil                  1.0 / 120.0, 7.0, 1
kenvelope            	transeg                 0.0, iattack, -7.0, 1.0, idecay, -7.0, 0.125, isustain, 0.0, 0.125, irelease, -7.0, 0.0
ag                      pluck                   kenvelope, cpsoct(ioctave + kvibrato), iHz, igenleft, 1
agleft                  pluck                   kenvelope, ihertzleft,  iHz, igenleft, 1
agright                 pluck                   kenvelope, ihertzright, iHz, igenright, 1
imsleft                 =                       0.2 * 1000
imsright                =                       0.21 * 1000
adelayleft              vdelay                  ag, imsleft, imsleft + 100
adelayright             vdelay                  ag, imsright, imsright + 100
asignal                 =                       kenvelope * (agleft + adelayleft + agright + adelayright)
                        ; Highpass filter to exclude speaker cone excursions.
asignal1                butterhp                asignal, 32.0
asignal2                balance                 asignal1, asignal
aleft, aright		    Pan			            p7, asignal2 * iamplitude
p3, aleft, aright	    Declick			        0       .006, p3, 0.06, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 21                ; Melody (Chebyshev / FM / additive), Jon Nelson
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -12.5
p3, adamping		    Damping			        0.01, p3, 0.01
ip3                     init                    3.0
iattack			        =			            0.05
isustain		        =			            p3
irelease		        =			            0.1
ip6 			        = 			            gichebychev
                        ; Pitch.
i1                      =                       iHz
k100                    randi                   1,10
k101                    poscil                  1, 5 + k100, gisine
k102                    linseg                  0, .5, 1, p3, 1
k100                    =                       i1 + (k101 * k102)
                        ; Envelope for driving oscillator.
; k1                      linenr                  0.5, ip3 * .3, ip3 * 2, 0.01
k1                      linseg                  0, ip3 * .3, .5, ip3 * 2, 0.01, isustain, 0.01, irelease, 0
; k2                      line                    1, p3, .5
k2                      linseg                  1.0, ip3, .5, isustain, .5, irelease, 0
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
a4                      foscili                  1, k100 + .04, 1, 2.005, k20, gisine
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 26
iphase			        =			            p2
ikey                    =                       12 * int(p4 - 6) + 100 * (p4 - 6)
ifqc                    =                       iHz
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
p3, aleft, aright	    Declick			        0.025, p3, .15, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

                        instr 23                ; FM Bell
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 9
kc1                     =                       5
kc2                     =                       5
kvdepth                 =                       0.0125
kvrate                  =                       5.1
ifn1                    =                       1
ifn2                    =                       1
ifn3                    =                       1
ifn4                    =                       1
ivfn                    =                       1
aout	                fmbell	                iamplitude, iHz, kc1, kc2, kvdepth, kvrate, ifn1, ifn2, ifn3, ifn4, ivfn
aenv                    transeg                 0.0, .001, -6, 1.0, 9, -6, 0
aout                    =                       aout * aenv
aleft, aright		    Pan			            p7, aout
p3, aleft, aright	    Declick			        0.001, p3, .5, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.1, 1.0
                        SendOut			        p1, aleft, aright
                        endin
            
                        instr 24                ; STK BandedWG
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -33.5
asignal 		        STKBandedWG 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.006, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin
            
                        instr 25                ; STK BeeThree
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -19
asignal 		        STKBeeThree 		    iHz, 1.0, 1, 1.5, 2, 4.8, 4, 2.1
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -2
asignal 		        STKBlowBotl 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 27                ; STK BlowHole
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -2.55
asignal 		        STKBlowHole 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 28                ; STK Bowed
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -19.5
                                                ; Controllers: 
                                                ;   1  Vibrato Gain
                                                ;   2  Bow Pressure
                                                ;   4  Bow Position
                                                ;  11  Vibrato Frequency
                                                ; 128  Volume 
asignal 		        STKBowed 		        iHz, 1.0, 1, 0.8, 2, 100.0, 4, 50.0, 11, 20.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.6, 0.0, 0.8, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 29                ; STK Brass TODO: Fix this
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
                                                ; Control Change Numbers: 
                                                ;    - Lip Tension = 2
                                                ;    - Slide Length = 4
                                                ;    - Vibrato Frequency = 11
                                                ;    - Vibrato Gain = 1
                                                ;    - Volume = 128
asignal 		        STKBrass 		        iHz, 1.0, 2, .5, 4, 12.0, 11, 6.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 30                ; STK Clarinet
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -5
asignal 		        STKClarinet 		    iHz, 1.0, 1, 1.5
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 31                ; STK Drummer
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -4.7
asignal 		        STKDrummer 		        iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 32                ; STK Flute
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -9
                                                ; Control Change Numbers:
                                                ;    * Jet Delay = 2
                                                ;    * Noise Gain = 4
                                                ;    * Vibrato Frequency = 11
                                                ;    * Vibrato Gain = 1
                                                ;    * Breath Pressure = 128
asignal 		        STKFlute 		        iHz, 1.0, 128, 90, 2, 80
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 33                ; STK FMVoices
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -3.5
                                                ; Control Change Numbers:
                                                ;    * Vowel = 2
                                                ;    * Spectral Tilt = 4
                                                ;    * LFO Speed = 11
                                                ;    * LFO Depth = 1
                                                ;    * ADSR 2 & 4 Target = 128
asignal 		        STKFMVoices 		    iHz, 1.0, 4, 12.0, 11, 5, 1, .8
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 34                ; STK HevyMetl
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -13.5
                                                ; Control Change Numbers:
                                                ;    * Total Modulator Index = 2
                                                ;    * Modulator Crossfade = 4
                                                ;    * LFO Speed = 11
                                                ;    * LFO Depth = 1
                                                ;    * ADSR 2 & 4 Target = 128
asignal 		        STKHevyMetl 		    iHz, 1.0, 2, 7.0, 4, 15
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 35                ; STK Mandolin
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -5
asignal 		        STKMandolin 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 36                ; STK ModalBar
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -9
                                                ; Control Change Numbers:
                                                ;    * Stick Hardness = 2
                                                ;    * Stick Position = 4
                                                ;    * Vibrato Gain = 1
                                                ;    * Vibrato Frequency = 11
                                                ;    * Direct Stick Mix = 8
                                                ;    * Volume = 128
                                                ;    * Modal Presets = 16
                                                ;          o Marimba = 0
                                                ;          o Vibraphone = 1
                                                ;          o Agogo = 2
                                                ;          o Wood1 = 3
                                                ;          o Reso = 4
                                                ;          o Wood2 = 5
                                                ;          o Beats = 6
                                                ;          o Two Fixed = 7
                                                ;          o Clump = 8
asignal 		        STKModalBar 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 37                ; STK Moog
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -16
asignal 		        STKMoog 		        iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 38                ; STK PercFlut
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -15
asignal 		        STKPercFlut 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 39                ; STK Plucked
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -6
asignal 		        STKPlucked 		        iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 40                ; STK Resonate
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
                                                ;Control Change Numbers:
                                                ;    * Resonance Frequency (0-Nyquist) = 2
                                                ;    * Pole Radii = 4
                                                ;    * Notch Frequency (0-Nyquist) = 11
                                                ;    * Zero Radii = 1
                                                ;    * Envelope Gain = 128
asignal 		        STKResonate 		    iHz, 1.0, 2, 4, 4, .98, 11, 12, 1, .5
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 41                ; STK Rhodey
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -10
asignal 		        STKRhodey 		        iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 42                ; STK Saxofony
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
                                                ; Control Change Numbers:
                                                ;    * Reed Stiffness = 2
                                                ;    * Reed Aperture = 26
                                                ;    * Noise Gain = 4
                                                ;    * Blow Position = 11
                                                ;    * Vibrato Frequency = 29
                                                ;    * Vibrato Gain = 1
                                                ;    * Breath Pressure = 128
asignal 		        STKSaxofony 		    iHz, 1.0, 2, 80, 11, 80, 29, 5, 1, 12
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 43                ; STK Shakers
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -31
                                                ;Control Change Numbers:
                                                ;    * Shake Energy = 2
                                                ;    * System Decay = 4
                                                ;    * Number Of Objects = 11
                                                ;    * Resonance Frequency = 1
                                                ;    * Shake Energy = 128
                                                ;    * Instrument Selection = 1071
                                                ;          o Maraca = 0
                                                ;          o Cabasa = 1
                                                ;          o Sekere = 2
                                                ;          o Guiro = 3
                                                ;          o Water Drops = 4
                                                ;          o Bamboo Chimes = 5
                                                ;          o Tambourine = 6
                                                ;          o Sleigh Bells = 7
                                                ;          o Sticks = 8
                                                ;          o Crunch = 9
                                                ;          o Wrench = 10
                                                ;          o Sand Paper = 11
                                                ;          o Coke Can = 12
                                                ;          o Next Mug = 13
                                                ;          o Penny + Mug = 14
                                                ;          o Nickle + Mug = 15
                                                ;          o Dime + Mug = 16
                                                ;          o Quarter + Mug = 17
                                                ;          o Franc + Mug = 18
                                                ;          o Peso + Mug = 19
                                                ;          o Big Rocks = 20
                                                ;          o Little Rocks = 21
                                                ;          o Tuned Bamboo Chimes = 22
asignal 		        STKShakers 		        iHz, 1.0, 1071, 22, 11, 1, 128, 100, 1, 30
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 44                ; STK Simple TODO: Needs work.
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -33
                                                ; Control Change Numbers:
                                                ;    * Filter Pole Position = 2
                                                ;    * Noise/Pitched Cross-Fade = 4
                                                ;    * Envelope Rate = 11
                                                ;    * Gain = 128
asignal 		        STKSimple 		        iHz, 1.0, 2, 120, 4, 50, 11, 3
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 45                ; STK Sitar
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -12
asignal 		        STKSitar 		        iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 46                ; STK StifKarp TODO: Fix funny harmonic?
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -3
                                                ; Control Change Numbers:
                                                ;    * Pickup Position = 4
                                                ;    * String Sustain = 11
                                                ;    * String Stretch = 1
asignal 		        STKStifKarp 		    iHz, 1.0, 4, 50, 11, 120, 1, 2
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 47                ; STK TubeBell
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -11
asignal 		        STKTubeBell 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 48                ; STK VoicForm
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -12
asignal 		        STKVoicForm 		    iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 49                ; STK Whistle
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
asignal 		        STKWhistle 		        iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

                        instr 50                ; STK Wurley
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -13
asignal 		        STKWurley 		        iHz, 1.0
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

#ifdef ENABLE_SOUNDFONTS

instr 51                ; FluidSynth Steinway
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
                        ; Use channel assigned in fluidload.
ichannel		        =			            0
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
                        fluidNote		        giFluidsynth, ichannel, ikey, p5
                        endin

                        instr 52                ; FluidSynth General MIDI
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
                        ; Use channel assigned in fluidload.
ichannel		        =			            1
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
ivelocity 		        = 			            dbamp(iamplitude)
                        fluidNote		        giFluidsynth, ichannel, ikey, p5
                        endin

                        instr 			        53 ; FluidSynth Marimba
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
                        ; Use channel assigned in fluidload.
ichannel		        =			            2
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
ivelocity 		        = 			            dbamp(iamplitude)
                        fluidNote		    giFluidsynth, ichannel, ikey, p5
                        endin

                        instr 54                ; FluidSynth Organ
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
                        ; Use channel assigned in fluidload.
ichannel		        =			            3
ioffset			        =			            ((sr / 44100) - 1) * 12
ikey	 		        = 			            p4 - ioffset
ikey 			        =			            p4
ivelocity 		        = 			            dbamp(iamplitude)
                        fluidNote		        giFluidsynth, ichannel, ikey, p5
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 13
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
ifqc                    init                    iHz ; cpspch(p5)
                                                ; note:delay time=2x length of string (time to traverse it)
idlt                    init                    1.0 / ifqc		
                        print                   iHz, ifqc, idlt
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
 
#ifdef ENABLE_PIANOTEQ

                        instr 56                ; Pianoteq
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
ichan                   init                    1.0
                        vstnote                 giPianoteq, ichan, p4, p5, p3
                        endin

#end

                        instr 57                ; Epicycloid or Spirograph curve, Mikelson TODO: Find better parameters?
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
ifqc                    init                    iHz
ifqci                   init                    iHz ; gi2fqc
; gi2fqc                  init                    ifqc
ip4                     init                    iamplitude
ia                      init                    2 ; p6
ib                      init                    8.5 ; p7
ihole                   init                    0.7 ; p8
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
ifqc                    init                    iHz        
ip4                     init                    iamplitude
ifqci                   init                    iHz
;ifqci                   init                    gi3fqc
;gi3fqc                  init                    ifqc
ia                      =                       0.6 ; p6
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 12
ifqc                    init                    iHz
ip4                     init                    iamplitude
iu                      init                    5 ; p6
iv                      init                    0.5 ; p7
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

                        instr 60                ; Low-level plucked string, Comajuncosas TODO: Fix this.
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 27
ip5                     =                       0.3
ip6                     =                       iamplitude
ipluck                  =                       ip5 ; p5; pluck position ( 0 to 1 )
ifreq                   =                       iHz ; cpspch(p4)
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 21
                        ; Initializations
ifqc                    =                       iHz
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -3
aflute1                 init                    0
ifqc                    =                       iHz ; cpspch(p5)
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -2
ip4                     =                       iamplitude
ip6                     =                       1.6
ip7                     =                       1000
ip8                     =                       0.2
ip9                     =                       gireedtable
areedbell               init                    0
ifqc                    =                       iHz; cpspch(p5)
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 52
ip4                     =                       iamplitude
ip5                     =                       iHz
ip6                     =                       gigrtab
ip7                     =                       giwintab
ip8                     =                       0.033
ip9                     =                       150
ip10                    =                       1.6
idur                    =                       p3
iamp                    =                       iamplitude ; p4
ifqc                    =                       iHz ; cpspch(p5)
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
aleft                   =                       aoutl * kamp * iamplitude
aright                  =                       aoutr * kamp * iamplitude
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -1
iattack 		        = 			            0.02
idecay                  =                       0.05
isustain		        =			            p3
irelease 		        = 			            0.25
ihertz                  =                       iHz
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 8.5
iattack 		        = 			            0.02
idecay                  =                       0.03
isustain		        =			            p3
irelease 		        = 			            0.25
kenvelope               transeg                 0.0, iattack, 2.5, iamplitude, isustain, 0.0, iamplitude, idecay, 2.5, 0.0
asignal                 poscil3                 kenvelope, iHz, gicosine
aleft, aright           reverbsc                asignal, asignal, 0.90, 10000, sr, 0.775 
aleft, aright		    Pan			            p7, (aleft + aright) * 2.0
p3, aleft, aright	    Declick			        0.003, p3, .05, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin
                        
                        instr 67                ; Reverb Sine 2, Michael Gogins
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        pset                    0, 0, 3600, 0, 0, 0, 0, 0, 0, 0, 0
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 4.5
iattack 		        = 			            0.006
idecay                  =                       0.03
isustain		        =			            p3
irelease 		        = 			            0.25
kenvelope               transeg                 0.0, iattack, -2.5, iamplitude, isustain, 0.0, iamplitude, idecay, 2.5, 0.0
asignal                 poscil3                 kenvelope, iHz, gicosine
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
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, 0
iattack			        =			            0.004
idecay				=				    8.0
isustain		        =			            p3
irelease		        =			            0.05
icarrier                =                       1
imodulator              =                       2.0
imodulatorAmplitude     =                       8
ifrequencyb             =                       iHz * 1.003
icarrierb               =                       icarrier * 1.004
aenvelope               transeg                 0.0, iattack, -9.0, 1.0, isustain, -5.0, 0.625,irelease, -4.0, 0.0
kfmenvelope             transeg                 0.0, iattack, -9.0, 1.5, isustain, -5.0, 0.525, irelease, -4.0, 0.0
                        ; Use poscil to get arate FM.
amodulator              poscil                  imodulatorAmplitude * kfmenvelope, iHz * imodulator, gisine  
amodl, amodr            reverbsc                amodulator, amodulator, 0.5, sr * 0.75
asignal                 poscil                  1.0, iHz * amodl, gisine  
asignal                 =                       asignal * aenvelope
aleft, aright		    Pan			            p7, asignal * iamplitude
aleft, aright		    Pan			            p7, asignal * iamplitude
p3, aleft, aright	    Declick			        iattack, p3, irelease, aleft, aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1
                        SendOut			        p1, aleft, aright
                        endin

#ifdef ENABLE_SOUNDFONTS

                        instr 190               ; Fluidsynth output
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ijunk			        = 			            p1 + p2 + p3 + p4 + p5
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -25
aleft, aright   	    fluidOut		        giFluidsynth
aleft			        = 			            iamplitude * aleft
aright			        =			            iamplitude * aright
                        AssignSend		        p1, 0.0, 0.0, 0.2, 1.0
                        SendOut			        p1, aleft, aright
                        endin

#end

#ifdef ENABLE_PIANOTEQ
                        instr 191               ; Pianoteq output
                        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ijunk			        = 			            p1 + p2 + p3 + p4 + p5
iHz,kHz,iamplitude,idB  NoteOn                  p4, p5, -102
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
i 210       0       -1      0.90    0.02  		13000

; Master output.
; Insno	    Start	Dur	Fadein	Fadeout
i 220       0       -1   0.1     0.1

; 5 minutes of real-time, e.g. MIDI, performance.
; Will be turned off by 'e' statement in score.

f 0 300
;i 17 1 5 60 127
</CsScore>
</CsoundSynthesizer>
