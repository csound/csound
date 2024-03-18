<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc for RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o ATSinterpread.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2021

ires system_i 1,{{ atsa fox.wav fox.ats }} ; ATSA wants a mono file!

instr 1	

ktime	line	0, p3, 2.65
	ATSbufread ktime, 1, "fox.ats", p4
kamp	ATSinterpread 	p5  ; get envelope from partial
aosc	poscil3	kamp, p5
	outs	aosc * 8, aosc * 8

endin

</CsInstruments>
<CsScore>
;              partial     index         
s
i 1 0 2.65      12          1000
i 1 3 2.65      12          150
i 1 6 2.65      12          10000
s
i 1 0 2.65      72          1000
i 1 3 2.65      72          150
i 1 6 2.65      72          10000
e
</CsScore>
</CsoundSynthesizer>
