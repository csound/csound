<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o <=.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kval    randomh 0, 1.2, 20		;choose between 0 and 1.2

if kval >0 && kval<=.5 then		;3 possible outcomes
	kvl = 1			
elseif kval >.5 && kval<=1 then
	kvl =2
elseif kval >1 then
	kvl =3
endif

printks "random number = %f, result = %f\n", .1, kval, kvl
asig    poscil .7, 440*kvl, 1
        outs asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 16384 10 1

i 1 0 5
e
</CsScore>
</CsoundSynthesizer>

