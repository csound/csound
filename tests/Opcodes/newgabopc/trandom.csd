<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o trandom.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

seed 0				; every run time different values	

instr 1	
			
kmin init 0			;random number between 0 and 220
kmax init 220
ktrig = p4
k1   trandom ktrig, kmin, kmax
     printk2 k1			;print when k1 changes
asig poscil .4, 220+k1, 1	;if triggered, add random values to frequency
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 4096 10 1

i 1 0 2 0	;not triggered
i 1 + 2 1	;triggered
e
</CsScore>
</CsoundSynthesizer>
