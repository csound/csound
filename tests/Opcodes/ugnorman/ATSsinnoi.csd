<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ATSsinnoi.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel - 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

ires1 system_i 1,{{ atsa -h.5 -c1 drumsMlp.wav drumsMlp1.ats }}
ires2 system_i 1,{{ atsa  drumsMlp.wav drumsMlp2.ats }} ; default settings -h.25 -c4
 

instr 1	

ktime	line	0,  p3, 2

aout	ATSsinnoi 	ktime, 1, 1, 1, p4, p5
	outs	aout*.4, aout*.4		
endin

</CsInstruments>
<CsScore>
s
;			atsfile		partial
i 1 0 2 "drumsMlp1.ats"		3
i 1 3 2 "drumsMlp2.ats"		3
s
i 1 0 2 "drumsMlp1.ats"		13
i 1 3 2 "drumsMlp2.ats"		13
s
i 1 0 2 "drumsMlp1.ats"		30
i 1 3 2 "drumsMlp2.ats"		30

e
</CsScore>
</CsoundSynthesizer>
