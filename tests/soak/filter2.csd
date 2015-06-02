<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o filter2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	 ; 2 saw waves of which one is slightly detuned
 
ib1   = p5  
ivol  = p6				;volume to compensate                                               
kcps  init cpspch(p4)
asig1 vco2 .05, kcps			;saw 1
asaw1 filter2 asig1, 1, 1, 1, ib1	;filter 1 
asig2 vco2 .05, kcps+1			;saw 2                      
asaw2 filter2 asig2, 1, 1, 1, ib1	;filter 2
aout  = (asaw1+asaw2)*ivol		;mix
      outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 4 6.00 -.001 5	;different filter values
i 1 + 4 6.00 -.6   2	;and different volumes
i 1 + 4 6.00 -.95 .3	;to compensate
e
</CsScore>
</CsoundSynthesizer>
