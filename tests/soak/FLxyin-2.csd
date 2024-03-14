<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
</CsOptions>
<CsInstruments>

sr=44100
kr=441
ksmps=100
nchnls=2

; Example by Gabriel Maldonado

	FLpanel	"Move the mouse inside this panel to hear the effect",400,400
	FLpanel_end
	FLrun

	instr 1

k1, k2, kinside	FLxyin   50, 1000, 50, 1000, 100, 300, 50, 250, -2,-3
;if k1 <= 50 || k1 >=5000 || k2 <=100 || k2 >= 8000 kgoto end ; if cursor is outside bounds, then don't play!!!

a1	oscili	3000, k1, 1
a2	oscili	3000, k2, 1

	outs	a1,a2
printk2 k1
printk2 k2, 10
printk2 kinside, 20
end:
	endin
	
</CsInstruments>
<CsScore>

f1 0 1024 10 1
f2 0 17 19 1 1 90 1
f3 0 17 19 2 1 90 1
i1 0 3600

</CsScore>
</CsoundSynthesizer>
