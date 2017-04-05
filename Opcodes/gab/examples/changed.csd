<CsoundSynthesizer>
<CsOptions>
-odac -B441 -b441
</CsOptions>
<CsInstruments>

sr     =        44100
kr     =        100
ksmps  =        441
nchnls =        2

gkvar init 0

       instr    1
ktrig changed gkvar
printk2 ktrig
kvar init 0
if ktrig = 0 goto noevent
printk 0,kvar
kvar = kvar+5
noevent:

        endin

        instr   2
gkvar = gkvar +1
turnoff
        endin

</CsInstruments>
<CsScore>
i 1 0 20
i 2 1 1
i 2 3 1
i 2 4 1
i 2 8 1
i 2 12 1

</CsScore>
</CsoundSynthesizer>
