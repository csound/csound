<CsoundSynthesizer>
<CsOptions>
-odac -B441 -b441
</CsOptions>
<CsInstruments>

sr     =        44100
kr     =        100
ksmps  =        441
nchnls =        2

gkindx init -1

       instr    1
kindex init 0
ktrig metro 0.5
if ktrig = 0 goto noevent
gkindx = gkindx + 1
noevent:

	endin

	instr	2
kout1 init 0
kout2 init 0
kout3 init 0
kout4 init 0
vtablek  gkindx, 1, 1, 0, kout1,kout2, kout3, kout4
printk2 kout1
printk2 kout2
printk2 kout3
printk2 kout4
	endin

</CsInstruments>
<CsScore>
f 1 0 32 10 1
i 1 0 20
i 2 0 20
</CsScore>
</CsoundSynthesizer>
