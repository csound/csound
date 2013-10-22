<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen25.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisin ftgen 1, 0, 32768, 10, 1
gienv ftgen 2, 0, 1025, 25, 0, 0.01, 200, 1, 400, 1, 513, 0.01	; y value must be >= 0
   
  
instr 1

kcps init 3/p3			;play 3x over duration of note
kndx phasor kcps
ixmode = 1			;normalize to 0-1
kval table kndx, gienv, ixmode
kval =kval*100			;scale up to 0-100
asig poscil 1, 220+kval, gisin	;use table for amplitude
     outs asig, asig
  
endin
</CsInstruments>
<CsScore>

i 1 0 4

e
</CsScore>
</CsoundSynthesizer>

