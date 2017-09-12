<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o /.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idiv  = 1/p3 * p4
ktrm  oscil 1, idiv, 1						;use oscil as an envelope
printf "retrigger rate per note duration = %f\n",1, idiv
kndx  line 5, p3, 1						;vary index of FM
asig  foscil ktrm, 200, 1, 1.4, kndx, 1
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f 1  0 4096 10   1    ;sine wave

i 1 0 3	10   
i 1 4 3 15  	
i 1 8 3 2 

e
</CsScore>
</CsoundSynthesizer>

