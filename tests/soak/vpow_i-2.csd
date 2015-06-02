<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vpow_i-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gifil ftgen 1, 0, 0, 1, "fox.wav", 0, 0, 1

instr 1

ival       = p4				;different distortion settings
ielements  = p5
idstoffset = p6				;index offset
vpow_i 1, ival, ielements, idstoffset
asig lposcil 1, 1, 0, 0, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
	
i1	0	2.7	.5	70000	0	;no offset
i1	3	2.7	.01	50000	70000	;add another period of distortion, starting at sample 70000	

e
</CsScore>
</CsoundSynthesizer>
