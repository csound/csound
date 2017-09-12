<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fmvoice.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2
0dbfs  = 1

instr 1

kfreq = 110
kvowel = p4	; p4 = vowel (0 - 64)
ktilt  = p5
kvibamt = 0.005
kvibrate = 6

asig fmvoice .5, kfreq, kvowel, ktilt, kvibamt, kvibrate
outs asig, asig

endin
</CsInstruments>
<CsScore>
;  sine wave.
f 1 0 16384 10 1

i 1 0 1 1  0	; tilt=0
i 1 1 1 >  .
i 1 2 1 >  .
i 1 3 1 >  .
i 1 4 1 >  .
i 1 5 1 >  .
i 1 6 1 >  .
i 1 7 1 12 .

i 1 10 1 1  90	; tilt=90
i 1 11 1 >  .
i 1 12 1 >  .
i 1 13 1 >  .
i 1 14 1 >  .
i 1 15 1 >  .
i 1 16 1 >  .
i 1 17 1 12 .

e
</CsScore>
</CsoundSynthesizer>
