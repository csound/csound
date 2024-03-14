<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ATSreadnz.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2023

ires system_i 1,{{ atsa Mathews.wav Mathews.ats }} ; default settings

instr 1	

ktime	line	0, p3, 2
kenergy	ATSreadnz ktime, "Mathews.ats", p4 ; return energy from band p4
anoise	randi	kenergy, 5000          
	outs	anoise *p5, anoise *p5		; compensate amplitude differences!
endin

</CsInstruments>
<CsScore>
; 3 different energy bands, compensated by different amplitude values
i 1 0  10      6	.015	; loud!!
i 1 11 10      12 	.4	; less energy in this band		
i 1 22 10      17 	1	; and even less...
e

</CsScore>
</CsoundSynthesizer>
