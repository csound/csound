<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

instr 1	

glid:k expon 440, p3, 880
sigLeft:a vco2 0.5, glid 
sigRight:a = vco2(0.5, 660)


signals:a[] init 4
signals[0] = sigLeft
signals[1] = sigRight
signals[2] = signals[0] 
signals[3] = signals[1] 
outs signals[2], signals[3] 

endin

</CsInstruments>
; ==============================================
<CsScore>

i1 0 .5

</CsScore>
</CsoundSynthesizer>

