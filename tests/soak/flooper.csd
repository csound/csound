<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o flooper.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kpitch	line	1, p3, .9 ;lower pitch a bit during the note
aout	flooper	.9, kpitch, 1, .53, 0.05, 1  ; loop starts at 1 sec, for .53 secs, 0.05 crossfade
	outs	aout, aout

endin
</CsInstruments>
<CsScore>
;table size is deferred,
; and format taken from the soundfile header
f 1 0 0 1 "fox.wav" 0 0 0

i 1 0 8.2
e
</CsScore>
</CsoundSynthesizer>
