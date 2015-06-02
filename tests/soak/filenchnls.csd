<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
;-odac     ;;;realtime audio out
-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o filechnls.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1 ; choose between mono or stereo file

ilen   filelen p4	;calculate length of soundfile
;print  ilen
ichn filenchnls  p4	;check number of channels
print  ichn

if (ichn == 1) then
;mono signal
asig diskin2 p4, 1	
     outs    asig, asig

else
;stereo signal
aL, aR diskin2 p4, .5, 0, 1 
       outs    aL, aR

endif

endin
</CsInstruments>
<CsScore>

i 1 0 3 "fox.wav"	;mono signal
i 1 5 2 "kickroll.wav"	;stereo signal

e
</CsScore>
</CsoundSynthesizer>
