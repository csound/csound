<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gensone.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	; simple oscillator with loudness correction. 

kcps = cpspch(p4) 
kenv linseg 0, p3*0.25, 1, p3*0.75, 0	;amplitude envelope
kamp tablei 16384 *kenv, 2 
asig oscil kamp, kcps, 1
     outs asig, asig
  
endin 

instr 2	;neutral oscillator to compare with

kcps = cpspch(p4)  
kenv linseg 0, p3*0.25, 1, p3*0.75, 0	;amplitude envelope
asig oscil kenv, kcps, 1
     outs asig, asig
  
endin 

</CsInstruments> 
<CsScore> 
f 1 0 16384 10 1 	;sine wave
f 2 0 16385 "sone" 0 32000 32000 0 

s
f 0 1	;1 second of silence before we start...
s 
i 1 0 2 7.00 
i 1 + . 7.01 
i 1 + . 8.02 
i 1 + . 8.03 
s 
i 2 0 2 7.00 
i 2 + . 7.01 
i 2 + . 8.02 
i 2 + . 8.03 
e 
</CsScore> 
</CsoundSynthesizer>

