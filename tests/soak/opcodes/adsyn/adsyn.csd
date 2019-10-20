<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
; analyze the file "kickroll.wav" first
kamod = 1
kfmod = p4
ksmod = p5

asig	adsyn	kamod, kfmod, ksmod, "kickroll.het"
	outs	asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 4  1 .2
i 1 + 1  2  1
i 1 + 1 .3 1.5
e

</CsScore>
</CsoundSynthesizer>
