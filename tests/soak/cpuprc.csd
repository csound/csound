<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o cpuprc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


cpuprc 1, 2
cpuprc 2, 30

instr 1 ;cpu processing-time percent usage is set to 2% for each note

asig oscil 0.2, 440, 1
     outs asig, asig

endin
 
instr 2	;cpu processing-time percent usage is set to 30% for each note
	;so the 4 notes of the score exceeds 100% by far
asig oscil 0.2, 440, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 32768 10 1	; sine wave

i 1 0 1
i 1 0 1
i 1 0 1
i 1 0 1

;too many notes to process,
;check Csound output!
i 2 3 1
i 2 3 1
i 2 3 1
i 2 3 1
e
</CsScore>
</CsoundSynthesizer>
