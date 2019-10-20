<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kacf = p4
kfcf = p5
asig soundin "fox.wav"
fsig pvsanal asig, 1024, 256, 1024, 1	; analyse it
ftps pvsmooth fsig, kacf, kfcf
atps pvsynth ftps			; synthesise it                      
     outs atps*3, atps*3

endin
</CsInstruments>
<CsScore>
;       amp  freq 
i 1 0 3 0.01 0.01	;smooth amplitude and frequency with cutoff frequency of filter at 1% of 1/2 frame-rate (ca 0.86 Hz)
i 1 + 3  1   0.01	;no smoothing on amplitude, but frequency with cf at 1% of 1/2 frame-rate (ca 0.86 Hz)
i 1 + 10 .001  1	;smooth amplitude with cf at 0.1% of 1/2 frame-rate (ca 0.086 Hz)
			;and no smoothing of frequency
e
</CsScore>
</CsoundSynthesizer>
