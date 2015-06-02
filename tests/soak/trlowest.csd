<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o trlowest.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ain     diskin2 "beats.wav", 1
fs1,fsi2 pvsifd ain, 2048, 512, 1	; ifd analysis
fst partials fs1, fsi2, .003, 1, 3, 500 ; partial tracking
flow,kfr,kamp trlowest fst, 1		; lowest freq-track 
aout	tradsyn	flow, 1, 1, 1, 1	; resynthesis of lowest frequency
	outs	aout*2, aout*2

endin
</CsInstruments>
<CsScore>
f1 0 8192 10 1	;sine wave

i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
