<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

gival = 333

instr 1	
ival = 44

pset 1, 0, 2, 200, ival, gival

print p4
print p5
print p6

endin

</CsInstruments>
; ==============================================
<CsScore>

i1 0 1
i1 0.5 1 555
i1 1 1 555 5555
i1 1.5 1 555 5555 555555

</CsScore>
</CsoundSynthesizer>

