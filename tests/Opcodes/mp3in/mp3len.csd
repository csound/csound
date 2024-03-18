<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o mp3len.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ilen   mp3len p4        ;calculate length of mp3 file
print  ilen

asigL, asigR mp3in p4
       outs  asigL, asigR

endin
</CsInstruments>
<CsScore>

i 1 0 30 "XORNOT_jul-14-05.mp3"    ; long signal
e
</CsScore>
</CsoundSynthesizer>
