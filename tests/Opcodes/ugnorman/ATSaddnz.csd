<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc for RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ATSaddnzwav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel - 2021

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

;ATSA wants a mono file!
ires system_i 1,{{ atsa Mathews.wav Mathews.ats }} ; default options

instr 1	

ktime	line     0, p3, p3
asig	ATSaddnz ktime, "Mathews.ats", 1, 4   ; only 1 noise band, the 4th noise band
	outs	asig*2, asig*2	;amplify
endin

</CsInstruments>
<CsScore>
i1 1 15.6 
e
</CsScore>
</CsoundSynthesizer>
