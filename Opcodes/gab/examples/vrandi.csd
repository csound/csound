<CsoundSynthesizer>
<CsOptions>
-odac1 -B441 -b441
</CsOptions>
<CsInstruments>

sr     =        44100
kr     =        100
ksmps  =        441
nchnls =        2

gindx init 0


	instr	2
kout1 init 0
kout2 init 0
kout3 init 0
kout4 init 0
indx = p4
vrandi   1, 5, 2, 1

vtablek 1, 1, 0, 0, kout1, kout2, kout3, kout4
printk 0.5, kout1
printk 0.5, kout2
printk 0.5, kout3
printk 0.5, kout4
	endin

</CsInstruments>
<CsScore>
f 1 0 32 10 1
i 2 0 20

</CsScore>
</CsoundSynthesizer>
