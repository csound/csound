<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o filenchnls.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2022

instr 1 ; choose between mono or stereo file

ichn filenchnls  p4	            ; check number of channels
prints  "\nnumber of channels = %d\n\n", ichn

if (ichn == 1) then
    asig diskin2 p4, 1              ; mono signal	
    outs    asig, asig
else
    aL, aR diskin2 p4, 1            ; stereo signal
    outs    aL, aR
endif

endin

</CsInstruments>
<CsScore>

i 1 0 3 "fox.wav"	;mono signal
i 1 5 4 "drumsSlp.wav"	;stereo signal
e
</CsScore>
</CsoundSynthesizer>
