<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o sqrt.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

asig   pluck 0.7, 55, 55, 0, 1
kpan   line 0,p3,1 
kleft  = sqrt(1-kpan) 
kright = sqrt(kpan) 
printks "square root of left channel = %f\\n", 1, kleft	;show coarse of sqaure root values
       outs asig*kleft, asig*kright					;where 0.707126 is between 2 speakers

endin 
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>