<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o loscil3.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ichnls = ftchnls(p4)
print ichnls

if (ichnls == 1) then
   asigL loscil3 .8, 1, p4, 1
   asigR = 	asigL
elseif (ichnls == 2) then
   asigL, asigR loscil3 .8, 1, p4, 1
;safety precaution if not mono or stereo
else
   asigL = 0
   asigR = 0
endif
        outs asigL, asigR

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "mary.wav" 0 0 0
f 2 0 0 1 "kickroll.wav" 0 0 0

i 1 0 3 1 ;mono file
i 1 + 2 2 ;stereo file
e
</CsScore>
</CsoundSynthesizer>
