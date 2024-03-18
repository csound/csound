<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in   No messages  MIDI in
-odac           -iadc     -d         -M0  ;;;RT audio I/O with MIDI in
; For Non-realtime ouput leave only the line below:
; -o polyaft.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr	=  44100
ksmps	=  10
nchnls	=  1

	massign 1, 1
itmp	ftgen 1, 0, 1024, 10, 1		; sine wave

	instr 1

kcps	cpsmidib 2		; note frequency
inote	notnum			; note number
kaft	polyaft inote, 0, 127	; aftertouch
; interpolate aftertouch to eliminate clicks
ktmp	phasor 40
ktmp	trigger 1 - ktmp, 0.5, 0
kaft	tlineto kaft, 0.025, ktmp
; map to sine curve for crossfade
kaft	=  sin(kaft * 3.14159 / 254) * 22000

asnd	oscili kaft, kcps, 1

	out asnd

	endin


</CsInstruments>
<CsScore>

t 0 120
f 0 9 2 -2 0
e


</CsScore>
</CsoundSynthesizer>
