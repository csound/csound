<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -Q1 -M0  ;;;realtime audio out and midi in and out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outiat.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 10
nchnls = 2

instr 1

ikey notnum 
ivel  veloc

ivib = 25		;low value.
outiat 1, ivib, 0, 127	;assign aftertouch on
print ivib		;external synth for example to
midion 1, ikey, ivel	;change depth of filter modulation

endin
</CsInstruments>
<CsScore>
f0 30 			;play for 30 seconds

e
</CsScore>
</CsoundSynthesizer>
