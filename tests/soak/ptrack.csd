<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ptrack.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

ihop  = p4
aout  diskin2 "fox.wav",1, 0, 1
kf,ka ptrack aout, ihop	; pitch track with different hopsizes
kcps  port kf, 0.01	; smooth freq
kamp  port ka, 0.01	; smooth amp
; drive an oscillator
asig  poscil ampdb(kamp)*0dbfs, kcps, 1
      outs  asig, asig

endin
</CsInstruments>
<CsScore>
; simple sine wave
f 1 0 4096 10 1

i 1 0  5 128
i 1 6  5 512
i 1 12 5 1024
e
</CsScore>
</CsoundSynthesizer>
