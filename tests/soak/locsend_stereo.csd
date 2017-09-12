<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o locsend_stereo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

ga1 init 0
ga2 init 0

instr 1

krevsend  = p4
aout	  diskin2 "beats.wav", 1, 0, 1
kdegree	  line 0, p3, 180 ;left to right
kdistance line 1, p3, 30
a1, a2	  locsig aout, kdegree, kdistance, p4
ar1, ar2  locsend
ga1 = ga1+ar1
ga2 = ga2+ar2
          outs a1, a2

endin

instr 99 ; reverb instrument
a1	reverb2 ga1, 2.5, .5
a2	reverb2 ga2, 2.5, .5
	outs	a1, a2
ga1 = 0
ga2 = 0

endin
</CsInstruments>
<CsScore>
; sine wave.
f 1 0 16384 10 1

i 1 0 4 .1	;with reverb
i 1 + 4 0	;no reverb
i99 0 7
e
</CsScore>
</CsoundSynthesizer>
