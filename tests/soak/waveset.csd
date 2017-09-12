<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o waveset.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs   =1

instr 1

krep init p4
asig soundin "flute.aiff"
aout waveset asig, krep
     outs aout, aout
  
endin
</CsInstruments>
<CsScore>

i 1 0 3 1	;no repetitions
i 1 + 10 3	;stretching 3 times
i 1 + 14 6	;6 times

e
</CsScore>
</CsoundSynthesizer>
