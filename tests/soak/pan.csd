<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pan.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4
0dbfs  = 1

instr 1

kcps = p4
k1   phasor kcps		; "fraction" of circle - controls speed of rotation - can be negative
k2   tablei k1, 1, 1 		; sin of angle (sinusoid in f1)
k3   tablei k1, 1, 1, .25, 1	; cos of angle (sin offset 1/4 circle)
arnd randomi 400, 1000, 50	; produce random values 
asig poscil .7, arnd, 1		; audio signal.. 

a1,a2,a3,a4 pan	asig, k2/2, k3/2, 2, 1, 1	; sent in a circle (f2=1st quad sin)
	outq	a1, a2, a3, a4

endin
</CsInstruments>
<CsScore>

f1 0 8192 10 1
f2 0 8193 9 .25 1 0

i1 0 10 .2	;move to the tight
i1 11 10 -.2	;move to the left
e

</CsScore>
</CsoundSynthesizer>
