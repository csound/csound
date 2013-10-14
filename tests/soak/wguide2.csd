<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o wguide2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr	1

aout diskin2 "beats.wav", 1, 0, 1				;in signal
afreq1 line 100, p3, 2000						
afreq2 line 1200, p3, p4					;vary second frequency in the score	
kcutoff1   = 3000
kcutoff2   = 1500
kfeedback1 = 0.25						;the sum of the two feedback
kfeedback2 = 0.25						;values should not exceed  0.5
asig wguide2 aout, afreq1, afreq2, kcutoff1, kcutoff2, kfeedback1, kfeedback2
asig dcblock2 asig						;get rid of DC
      outs asig, asig 
      
endin
</CsInstruments>
<CsScore>
i 1 0 8 1200	;freqency of afreq2 remains the same
i 1 9 8 100	;freqency of afreq2 gets lower
e
</CsScore>
</CsoundSynthesizer>

