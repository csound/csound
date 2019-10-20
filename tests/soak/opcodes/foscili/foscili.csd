<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

kcps = 440
kcar = 1
kmod = p4
kndx line 0, p3, 20	;intensivy sidebands

asig foscili .5, kcps, kcar, kmod, kndx, 1
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; sine
f 1 0 16384 10 1

i 1 0  9 .01	;vibrato
i 1 10 .  1
i 1 20 . 1.414	;gong-ish
i 1 30 5 2.05	;with "beat"
e
</CsScore>
</CsoundSynthesizer>
