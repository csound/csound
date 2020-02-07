<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o loscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

   asigL loscil .8, 1, p4, 1
   asigR loscil .8, 1.2, p4, 1
        outs asigL, asigR

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "mary.wav" 0 0 0

i 1 0 3 1 ;mono file
e
</CsScore>
</CsoundSynthesizer>
