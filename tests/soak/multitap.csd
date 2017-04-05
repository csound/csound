<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o multitap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

ga1 init 0

instr 1

asig diskin2 "beats.wav", 1,0
     outs asig, asig
   
ga1  = ga1+asig
endin

instr 2

asig multitap ga1, 1.2, .5, 1.4, .2
     outs     asig, asig
	
ga1  = 0
endin

</CsInstruments>
<CsScore>

i 1 .5 .2	; short sound
i 2  0  3	; echoes
e
</CsScore>
</CsoundSynthesizer>
