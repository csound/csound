<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vlowres.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1

kamp init p4
asig vco2  kamp, 110			;saw wave
kfco line 30, p3, 300			;vary the cutoff frequency from 30 to 300 Hz.
kres = 20	
ksep = p5				;different resonance values
iord = p6				;and different number of filters
aout vlowres asig, kfco, kres, iord, ksep
aclp clip aout, 1, 1			;avoid distortion
     outs aclp, aclp
     
endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine

s
i 1 0 10 .1 5  2	;compensate volume and 
i 1 + 10 .1 25 2	;number of filters = 2
s
i 1 0 10 .01 5  6	;compensate volume and 
i 1 + 10 .04 15 6	;number of filters = 6

e
</CsScore>
</CsoundSynthesizer>

