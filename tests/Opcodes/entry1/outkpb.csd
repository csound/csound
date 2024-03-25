<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac -Q1 -M0  ;;;realtime audio out and midi in and out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o outkpb.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 10
nchnls = 2

instr 1

ikey notnum 
ivel  veloc

kpch linseg 100, 1, 0	;vary in 1 second
kpb = int(kpch)		;whole numbers only
outkpb 1, kpb, 0, 127	;(= pitchbend)
midion 1, ikey, ivel	;of external synth

endin
</CsInstruments>
<CsScore>
f0 30

e
</CsScore>
</CsoundSynthesizer>
