<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o flanger.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

kfeedback = p4
asnd vco2 .2, 50
adel linseg 0, p3*.5, 0.02, p3*.5, 0	;max delay time =20ms	
aflg flanger asnd, adel, kfeedback
asig clip aflg, 1, 1
     outs asig+asnd, asig+asnd		;mix flanger with original

endin
</CsInstruments>
<CsScore>

i 1 0 10 .2
i 1 11 10 .8	;lot of feedback
e
</CsScore>
</CsoundSynthesizer>
