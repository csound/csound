<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -Q1 -M0  ;;;realtime audio out -+rtmidi=virtual
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outic.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1

ikey notnum 
ivel veloc
kbrt = 40			;set controller 74 (=brightness)			
outic 1, 74, kbrt, 0, 127	;so filter closes a bit
midion 1, ikey, ivel		;play external synth

endin
</CsInstruments>
<CsScore>
f0 30	;runs 30 seconds

e
</CsScore>
</CsoundSynthesizer>
