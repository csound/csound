<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o seqtime2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gitabMap2 ftgen	57,0,512,-2, 1,1/4,1/4,1/8,1/8,1/8,1/8,1/4,1/4,.5,1/4,1/4,1/16,1/16,1/16,1/16,1/16,1/16,1/16,1/16
gisine	  ftgen	1,0,512,10, 1

instr 1

ktrigin	metro	.333333333333
ktrig2	metro	1
	schedkwhen ktrig2, 0,0, 2, 0, .1			; just to set the metronome!
kspeed	init	1
;          	 ktime_unit, kstart, kloop, initndx, kfn_times 
ktrig	seqtime2 ktrigin, kspeed, 0, 20, 2, gitabMap2
;ktrig	seqtime	 kspeed, 0, 20, 0, gitabMap2			; try with seqtime too...		
	schedkwhen ktrig, 0, 0, 3, 0, ktrig			; the duration is got from seqtime2 output!
endin

instr 2

a1	line	1,p3,0
aout	oscili	0.7*a1,500,gisine
	outs1	aout
endin	


instr 3

a1	line 	1,p3,0
aout	oscili	0.7*a1,1000,gisine
	outs2	aout
endin	

</CsInstruments>
<CsScore>
i1 0 20

;f0 3600
</CsScore>
</CsoundSynthesizer>

