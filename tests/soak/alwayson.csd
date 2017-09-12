<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
;-Wfo alwayson.wav 
</CsOptions> 
<CsInstruments> 

; Initialize the global variables. 

sr     = 44100 
ksmps 	= 32 
nchnls 	= 2 

; Connect up instruments and effects to create the signal flow graph. 

connect "SimpleSine",   	"leftout",     "Reverberator",     	"leftin" 
connect "SimpleSine",   	"rightout",    "Reverberator",     	"rightin" 

connect "Moogy",        	"leftout",     "Reverberator",     	"leftin" 
connect "Moogy",        	"rightout",    "Reverberator",     	"rightin" 

connect "Reverberator", 	"leftout",     "Compressor",       	"leftin" 
connect "Reverberator", 	"rightout",    "Compressor",       	"rightin" 

connect "Compressor",   	"leftout",     "Soundfile",       	"leftin" 
connect "Compressor",   	"rightout",    "Soundfile",       	"rightin" 

; Turn on the "effect" units in the signal flow graph. 

alwayson "Reverberator", 0.91, 12000 
alwayson "Compressor" 
alwayson "Soundfile" 

; Define instruments and effects in order of signal flow.

			    instr SimpleSine 
                ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			    ; Default values:   p1  p2  p3  p4  p5  p6  p7  p8  p9  p10
			    pset			    0,  0,  10, 0,  0,  0,  0.5
iattack			=			        0.015
idecay			=			        0.07
isustain		=			        p3
irelease		=			        0.3
p3			    =			        iattack + idecay + isustain + irelease
adamping		linsegr			    0.0, iattack, 1.0, idecay + isustain, 1.0, irelease, 0.0
iHz 			= 			        cpsmidinn(p4) 
                ; Rescale MIDI velocity range to a musically usable range of dB. 
iamplitude 		= 			        ampdb(p5 / 127 * 15.0 + 60.0) 
			    ; Use ftgenonce instead of ftgen, ftgentmp, or f statement. 
icosine			ftgenonce 		    0, 0, 65537, 11, 1 
aoscili			oscili 			    iamplitude, iHz, icosine 
aadsr 			madsr 			    iattack, idecay, 0.6, irelease 
asignal 		= 			        aoscili * aadsr 
aleft, aright	pan2			    asignal, p7
			    ; Stereo audio output to be routed in the orchestra header. 
			    outleta 		    "leftout", aleft
			    outleta 		    "rightout", aright
			    endin 

			    instr Moogy 
                ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			    ; Default values:   p1  p2  p3  p4  p5  p6  p7  p8  p9  p10
			    pset			    0,  0,  10, 0,  0,  0,  0.5
iattack			=			        0.003
isustain		=			        p3
irelease		=			        0.05
p3			    =			        iattack + isustain + irelease
adamping		linsegr			    0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iHz 			= 			        cpsmidinn(p4)
                ; Rescale MIDI velocity range to a musically usable range of dB. 
iamplitude 		= 			        ampdb(p5 / 127 * 20.0 + 60.0) 
			    print 			    iHz, iamplitude 
			    ; Use ftgenonce instead of ftgen, ftgentmp, or f statement. 
isine 			ftgenonce 		    0, 0, 65537, 10, 1 
asignal 		vco 			    iamplitude, iHz, 1, 0.5, isine 
kfco 			line 			    2000, p3, 200
krez 			= 			        0.8 
asignal 		moogvcf 		    asignal, kfco, krez, 100000 
asignal			=			        asignal * adamping
aleft, aright	pan2			    asignal, p7
			    ; Stereo audio output to be routed in the orchestra header. 
			    outleta 		    "leftout", aleft
			    outleta 		    "rightout", aright 
			    endin 
			    instr Reverberator 
                ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			    ; Stereo input. 
aleftin 		inleta 			    "leftin" 
arightin 		inleta 			    "rightin" 
idelay 			= 			        p4 
icutoff 		= 			        p5 
aleft, aright 	reverbsc 	        aleftin, arightin, idelay, icutoff 
			    ; Stereo output. 
			    outleta 	        "leftout", aleft 
			    outleta 	        "rightout", aright 
			    endin 

			    instr Compressor 
                ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			    ; Stereo input. 
aleftin 		inleta 		        "leftin" 
arightin 		inleta 		        "rightin" 
kthreshold 		= 		            25000 
icomp1 			= 		            0.5 
icomp2 			= 		            0.763 
irtime 			= 		            0.1 
iftime 			= 		            0.1 
aleftout 		dam 		        aleftin, kthreshold, icomp1, icomp2, irtime, iftime 
arightout 		dam 		        arightin, kthreshold, icomp1, icomp2, irtime, iftime 
			    ; Stereo output. 
			    outleta 	        "leftout", aleftout 
			    outleta 	        "rightout", arightout 
			    endin 

			    instr Soundfile 
                ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
			    ; Stereo input. 
aleftin 		inleta 		        "leftin" 
arightin 		inleta 		        "rightin" 
			    outs 		        aleftin, arightin 
			    endin 

</CsInstruments> 
<CsScore> 

; It is not necessary to activate "effects" or create f-tables in the score! 
; Overlapping notes create new instances of instruments with proper connections. 

i "SimpleSine" 1 5 60 85 
i "SimpleSine" 2 5 64 80 
i "Moogy" 3 5 67 75 
i "Moogy" 4 5 71 70 
; 1 extra second after the performance
e 1 

</CsScore> 
</CsoundSynthesizer> 
