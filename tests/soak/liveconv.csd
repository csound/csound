<CsoundSynthesizer>
<CsOptions>
-odac  ;realtime audio out
</CsOptions>
<CsInstruments>

	sr	= 44100
	nchnls	= 2
	0dbfs	= 1

; empty IR table
giIR_record 	ftgen	0, 0, 131072, 2, 0
	
; Record impulse response
instr 13

p3 		=	ftlen(giIR_record)/sr
iskip 	=	p4
irlen 	=	p5
a1 		diskin2	"fox.wav", 1, iskip

; Fill IR table with segment from audio file
amp 	linseg	0, 0.1, 1, irlen, 1, 0.1, 0, 1, 0
andx_IR line	0, 1, 1/(ftlen(giIR_record)/sr)
		tablew	a1*amp, andx_IR, giIR_record, 1
		outch	1, a1*amp	; output the IR
ktrig 	init	1
if ktrig > -1 then
	chnset	ktrig, "conv_update"
	ktrig -= 1
endif

endin
        
; The convolver
instr 14

ain 	diskin2	"drumsMlp.wav", 1, 0, 1
kupdate chnget	"conv_update"
aconv 	liveconv ain, giIR_record, 2048, kupdate, 0
		outch	2, aconv*0.009	; output the convolution response
endin
        
        
</CsInstruments>
<CsScore>
; record impulse response
;          skip  IR_dur
i13	0	1	0.0	0.5
i13	2	1	0.5	0.5
i13	4	1	1.0	0.5
i13	6	1	1.5	0.5
i13	8	1	2.0	0.75
i13	10	1	2.38 0.25

; convolve
i14	0.0	11.65	

e

</CsScore>
</CsoundSynthesizer>

