<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o timedseq.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giseq ftgen 0,0,128,-2, 2,  0,   0.5, 8.00,\		;first note
                        2,  1,   0.5, 8.02,\		;second note
                        2,  2,   0.5, 8.04,\		;third
                        2,  3,   0.5, 8.05,\		;fourth
                        2,  4,   0.5, 8.07,\		;fifth
                        2,  5,   0.5, 8.09,\		;sixth
                        2,  6,   0.5, 8.11,\		;seventh
                        2,  7,   0.5, 9.00,\		;eight note
                        2,  8,   0.5, 8.00,\		;due to a quirk in the opcode, it needs an extra note - a copy of the first note
                        -1, 8,   -1,  -1		;last line is a dummy event that indicates to timedseq when to loop back to the beginning

instr	1

ibeats	= 8						;lengths of sequence in beats
itempo	= p4						;tempo
iBPS	= itempo/60					;beats per second	
kphase	phasor	iBPS/ibeats				;phasor to move through table
kpointer = kphase*ibeats				;multiply phase (range 0 - 1) by the number of beats contained within the sequence
kp1 init 0
kp2 init 0
kp3 init 0 
kp4 init 0       		
ktrigger   timedseq kpointer, giseq, kp1, kp2,kp3, kp4
schedkwhen ktrigger, 0, 0, 2, 0, kp3/abs(iBPS), kp4	;p3 values have been scaled according to tempo so that they maesure beats rather than seconds
endin							;abs(iBPS)(absolute value) is used because the tempo provided by the fourth note of the score is negative.
							;Durations here should be positive, because negative values for duration would indicate a held note.
instr	2

aenv	linseg	0,0.01,1,p3-0.01,0			;amplitude envelope
asig	vco2	0.4, cpspch(p4), 4, 0.5
	outs	asig*aenv, asig*aenv			
endin

</CsInstruments>
<CsScore>
i 1 0 4 120
i 1 + . 240
i 1 + . 480
i 1 + . -480	;when negative it plays backwards 
e
</CsScore>
</CsoundSynthesizer>

