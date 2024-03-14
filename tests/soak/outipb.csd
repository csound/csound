<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -Q1 -M0  ;;;realtime audio out and midi in and out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outipb.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 10
nchnls = 2

instr 1

ikey notnum 
ivel  veloc

ipb = 10		;a little out of tune
outipb 1, ipb, 0, 127	;(= pitchbend)
midion 1, ikey, ivel	;of external synth

endin
</CsInstruments>
<CsScore>
f0 30

e
</CsScore>
</CsoundSynthesizer>
