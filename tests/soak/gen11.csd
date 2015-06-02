<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen11.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
nchnls = 2 
0dbfs  = 1 

instr 1

ifn  = p4
asig oscil .8, 220, ifn
     outs asig,asig
    
endin
</CsInstruments>
<CsScore>
f 1 0 16384 11 1 1	;number of harmonics = 1
f 2 0 16384 11 10 1 .7	;number of harmonics = 10
f 3 0 16384 11 10 5 2	;number of harmonics = 10, 5th harmonic is amplified 2 times


i 1 0 2 1
i 1 + 2 2
i 1 + 2 3
e
</CsScore>
</CsoundSynthesizer>
