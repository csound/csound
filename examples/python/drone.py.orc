sr                      =           44100
ksmps                   =           441
nchnls                  =           2
0dbfs                   =           40000

; Bind named control channels to global variables.

gkDistortFactor         chnexport   "gkDistortFactor", 3
gkDistortFactor         init        1.0
gkReverbscFeedback      chnexport   "gkReverbscFeedback", 3
gkReverbscFeedback      init        0.9
gkMasterLevel           chnexport   "gkMasterLevel", 3
gkMasterLevel           init        1.0

gareverb1               init        0
gareverb2               init        0

			instr 1
iattack                 init        40.0
idecay                  init        40.0
isustain        	init        p3 - iattack
p3          		=           p3 + idecay
			print       p1, p2, p3, p4, p5, p6
ifundamental            =           p4  
inumerator              =           p5
idenominator            =           p6
ivelocity               =           p7
ipan                    =           p8
iratio                  =           inumerator / idenominator
ihertz                  =           ifundamental * iratio
iamp                    =           ampdb(ivelocity)
kenvelope               transeg     0.0, iattack / 2.0, 2.5, iamp / 2.0, iattack / 2.0, -2.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 2.5, iamp / 2.0, idecay / 2.0, -2.5, 0.0
asignal                 poscil3     kenvelope, ihertz, 1
asignal                 distort     asignal, gkDistortFactor, 2
aleft, aright           pan2        asignal , ipan
gareverb1               =           gareverb1 + aleft
gareverb2               =           gareverb1 + aright
			endin
            
            
			instr 30
aleft, aright           reverbsc    gareverb1, gareverb2, gkReverbscFeedback, 15000.0
aleft                   =           gkMasterLevel * (gareverb1 + aleft * 0.8)
aright                  =           gkMasterLevel * (gareverb2 + aright * 0.8)
			outs        aleft, aright
gareverb1               =           0
gareverb2               =           0
			endin

			instr 40
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
ipitchmod 		= 			0.98 
itone 			= 			16000     		
ain1 			=			gareverb1
ain2 			=			gareverb2
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
afilt1  		tone    		adel1 * gkReverbscFeedback, itone
afilt2  		tone    		adel2 * gkReverbscFeedback, itone
afilt3  		tone    		adel3 * gkReverbscFeedback, itone
afilt4  		tone    		adel4 * gkReverbscFeedback, itone
afilt5  		tone    		adel5 * gkReverbscFeedback, itone
afilt6  		tone    		adel6 * gkReverbscFeedback, itone
afilt7  		tone    		adel7 * gkReverbscFeedback, itone
afilt8  		tone    		adel8 * gkReverbscFeedback, itone
; The outputs of the delay lines are summed
; and sent to the stereo outputs. This could
; easily be modified for a 4 or 8-channel 
; sound system.
aout1 			= 			(afilt1 + afilt3 + afilt5 + afilt7)
aout2 			= 			(afilt2 + afilt4 + afilt6 + afilt8)
; To the master output.
aleft                   =           gkMasterLevel * (gareverb1 + aout1 * 0.8)
aright                  =           gkMasterLevel * (gareverb2 + aout2 * 0.8)
			outs        aleft, aright
gareverb1               =           0
gareverb2               =           0
			endin

