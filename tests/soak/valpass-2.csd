<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o valpass-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 65536, 10, 1			;sine wave

instr 1

asig diskin2 "beats.wav", 1, 0, 1
krvt line 0.01, p3, p3				;reverb time
adepth = p4					;sine depth 
krate = 0.3					;sine rate (speed)
adel oscil 0.5, krate, giSine			;delay time oscillator (LFO)
adel = ((adel+0.5)*adepth)			;scale and offset LFO
aout valpass asig, krvt, adel*0.01, 0.5
     outs aout, aout

endin
</CsInstruments>
<CsScore>
	
i1 0  10 1
i1 11 10 5	
e
</CsScore>
</CsoundSynthesizer>


