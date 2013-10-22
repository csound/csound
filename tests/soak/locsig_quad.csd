<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o locsig_quad.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 4
0dbfs  = 1

ga1 init 0
ga2 init 0
ga3 init 0
ga4 init 0

instr 1

krevsend  = p4
aout	  diskin2 "beats.wav", 1, 0, 1
kdegree	  line 0, p3, 360 ;full circle
kdistance line 1, p3 , 1
a1, a2, a3, a4     locsig aout, kdegree, kdistance, krevsend
ar1, ar2, ar3, ar4 locsend

ga1 = ga1+ar1
ga2 = ga2+ar2
ga3 = ga3+ar3
ga4 = ga4+ar4
    outq a1, a2, a3, a4

endin

instr 99 ; reverb instrument
a1 reverb2 ga1, 3.5, .5
a2 reverb2 ga2, 3.5, .5
a3 reverb2 ga3, 3.5, .5
a4 reverb2 ga4, 3.5, .5
   outq	a1, a2, a3, a4
	
ga1 = 0
ga2 = 0
ga3 = 0
ga4 = 0

endin
</CsInstruments>
<CsScore>
; sine wave.
f 1 0 16384 10 1

i 1 0  14  .1	;with reverb
i 1 14 14  0	;no reverb
i99 0 36
e
</CsScore>
</CsoundSynthesizer>
