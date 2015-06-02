<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o locsend_quad.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 4

ga1	init	0
ga2	init	0
ga3	init	0
ga4	init	0
; Instrument #1
instr 1
  ; Use the "beats.wav" audio file.
aout		diskin2 "beats.wav", 1, 0, 1
kdegree	line 0, p3, 360
kdistance	line 1, p3 , 10
a1, a2, a3, a4	locsig aout, kdegree, kdistance, .1
ar1, ar2, ar3, ar4	locsend
ga1	=	ga1+ar1
ga2	=	ga2+ar2
ga3	=	ga3+ar3
ga4	=	ga4+ar4

	outq a1, a2, a3, a4
endin

instr 99 ; reverb instrument
a1	reverb2	ga1, 3.5, .5
a2	reverb2	ga2, 3.5, .5
a3	reverb2	ga3, 3.5, .5
a4	reverb2	ga4, 3.5, .5

	outq	a1, a2, a3, a4
	
ga1 = 0
ga2 = 0
ga3 = 0
ga4 = 0

endin
</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

i 1 0 2
i99 0 5
e


</CsScore>
</CsoundSynthesizer>
