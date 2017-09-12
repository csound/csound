<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o port.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

aout  diskin2 "fox.wav",1, 0, 1
kf,ka ptrack aout, 512	; pitch track with winsize=1024
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

i 1 0  5
e
</CsScore>
</CsoundSynthesizer>
