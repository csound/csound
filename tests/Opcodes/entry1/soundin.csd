<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
;-odac     ;;;realtime audio out
-iadc   ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o soundin.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel 2022

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 ; choose between mono or stereo file

ichn filenchnls  p4	;check number of channels
prints  "\nnumber of channels = %d\n\n", ichn

if ichn == 1 then	
asig   soundin p4	;mono signal
       outs    asig, asig
else			;stereo signal
aL, aR soundin p4
       outs    aL, aR
endif

endin
</CsInstruments>
<CsScore>

i 1 0 2 "drumsMlp.wav"	;mono signal
i 1 3 2 "drumsSlp.wav"	;stereo signal

e
</CsScore>
</CsoundSynthesizer>
