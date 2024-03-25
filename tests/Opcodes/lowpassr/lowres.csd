<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lowres.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kres = p4
asig vco .2, 220, 1		;sawtooth

kcut line 1000, p3, 10		;note: kcut is not in Hz
as   lowres asig, kcut, kres	;note: kres is not in dB
aout balance as, asig		;avoid very loud sounds
     outs aout, aout

endin
</CsInstruments>
<CsScore>
; a sine
f 1 0 16384 10 1

i 1 0 4 3
i 1 + 4 30
i 1 + 4 60
e
</CsScore>
</CsoundSynthesizer>
