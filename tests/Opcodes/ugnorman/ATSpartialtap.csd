<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc for RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ATSpartialtap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2023

ires1 system_i 1,{{ atsa drumsMlp.wav drumsMlp1.ats }} ; default settings
ires2 system_i 1,{{ atsa -h.8 -c8 drumsMlp.wav drumsMlp2.ats }} ; longer hopsize & more cycles

instr 1	

ktime	line	0, p3, 2
	ATSbufread ktime, 1, p4 , 54
kfreq1, kam1	ATSpartialtap  5
kfreq2, kam2	ATSpartialtap  20
kfreq3, kam3	ATSpartialtap  30

aout1	oscil	kam1, kfreq1, 1
aout2	oscil	kam2, kfreq2, 1
aout3	oscil	kam3, kfreq3, 1
aout	=	(aout1+aout2+aout3)*10	; amplify some more
	outs	aout * p5, aout * p5
endin

</CsInstruments>
<CsScore>
f 1 0 16384 10 1 0 .5 0 .33 ; square-ish wave
; stretch the drums...
i 1 0 2 "drumsMlp1.ats"    2.5 ; defaults settings for ATSA
i 1 3 10 "drumsMlp2.ats"   1   ; different settings
e
</CsScore>
</CsoundSynthesizer>
