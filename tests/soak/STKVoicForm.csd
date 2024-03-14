<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o STKVoicForm.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ifrq	=	p4
kv2	line	p5, p3, p6				;Vowel/Phoneme Selection

asig	STKVoicForm cpspch(p4), 1, 2, 1, 4, kv2, 128, 100, 1, 10, 11, 100
asig	=	asig * .5				;too loud
	outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 5  7.00 100 0
i 1 + 10 7.00 1  50
e
</CsScore>
</CsoundSynthesizer>
