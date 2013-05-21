<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

sr	=	44100
ksmps	=	1
;nchnls	=	2
0dbfs	=	1

i1 init 0
lp:
i1 += 1
print i1

if(i1<10) goto lp

instr 1

endin

</CsInstruments>
<CsScore>
i1 0 1


</CsScore>
</CsoundSynthesizer>

