<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvsmix.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

asig1 diskin2 "drumsMlp.wav", .5,0,1		;signal in 1 at half speed
asig2 diskin2 "drumsMlp.wav", 1,0,1		;signal in 2
fsig1 pvsanal asig1,1024,256,1024,0	      ;pvoc analysis 
fsig2 pvsanal asig2,1024,256,1024,0	      ;of both signals
fsall pvsmix  fsig1, fsig2 
asig  pvsynth fsall
outs asig, asig 

endin 
</CsInstruments>
<CsScore>

i 1 0 8
e
</CsScore>
</CsoundSynthesizer>
