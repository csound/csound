<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o loscil.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 20222

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1     ; loscil makes use of embedded loop points in wav

ichnls = ftchnls(p4)
prints  "\nnumber of channels = %d\n\n", ichnls

if (ichnls == 1) then                   ; sample is played 2 x faster
   asigL loscil .8, 1.5, p4, 1          ; sample loops between 1 and end loop point at 2 secs. in sample
   asigR = 	asigL
elseif (ichnls == 2) then               ; sample is played at half speed
   asigL, asigR loscil .8, .5, p4, 1    ; sample loops between 2 and end loop point at 3 secs. in sample
else                                    ; safety precaution if not mono or stereo
   asigL = 0
   asigR = 0
endif
        outs asigL, asigR

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "drumsMlp.wav" 0 0 0
f 2 0 0 1 "drumsSlp.wav" 0 0 0

i 1 0 7 1   ;mono file
i 1 7 12 2  ;stereo file
e
</CsScore>
</CsoundSynthesizer>
