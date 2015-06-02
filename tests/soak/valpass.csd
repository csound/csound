<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o valpass.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

krvt = 1.5
klpt line p4, p3, p5
imaxlpt = .1

a1 diskin2 "fox.wav", 1
a1 valpass a1, krvt, klpt, imaxlpt
a2 valpass a1, krvt, klpt*.5, imaxlpt
   outs	a1, a2  

endin
</CsInstruments>
<CsScore>

i 1 0 5 .01 .2
e
</CsScore>
</CsoundSynthesizer> 
