<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -Q1 -M0  ;;;realtime audio out and midi in and out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outipc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1

outipc 1, 80, 0, 127	;program change --> 80
ikey notnum 
ivel veloc
midion 1, ikey, ivel	;play external synth

endin
</CsInstruments>
<CsScore>
f0 30	;runs 30 seconds

e
</CsScore>
</CsoundSynthesizer>
