<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o line.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

kp = p6
;choose between expon or line
if (kp == 0) then 	
  kpitch expon p4, p3, p5 
elseif (kp == 1) then
  kpitch line p4, p3, p5 
endif

asig   vco2 .6, kpitch 
       outs asig, asig

endin 
</CsInstruments> 
<CsScore> 

i 1 0  2 300  600  0	;if p6=0 then expon is used
i 1 3  2 300  600  1	;if p6=1 then line is used
i 1 6  2 600  1200 0
i 1 9  2 600  1200 1
i 1 12 2 1200 2400 0
i 1 15 2 1200 2400 1
i 1 18 2 2400 30   0
i 1 21 2 2400 30   1
e
</CsScore> 
</CsoundSynthesizer> 
