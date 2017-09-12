<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o locsig_stereo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

ga1	init	0
ga2	init	0
; Instrument #1
instr 1
  ; Use the "beats.wav" audio file.
aout		diskin2 "beats.wav", 1, 0, 1
kdegree	line 0, p3, 180
kdistance	line 1, p3 , 30
a1, a2		locsig aout, kdegree, kdistance, .1
ar1, ar2	locsend
ga1	=	ga1+ar1
ga2	=	ga2+ar2


  outs a1, a2
endin

instr 99 ; reverb instrument
a1	reverb2 ga1, 2.5, .5
a2	reverb2 ga2, 2.5, .5
	outs	a1, a2
ga1=0
ga2=0
endin
</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

; Play Instrument #1 for five seconds.
i 1 0 2
i99 0 5
e


</CsScore>
</CsoundSynthesizer>
