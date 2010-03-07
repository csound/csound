<CsoundSynthesizer>
<CsOptions>
csound -m255 -RWfo jacko_test.wav
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 128
nchnls = 2
0dbfs = 1

jackinit "default", "csound"
jackinfo
jackfreewheel 1

alwayson "jackin"

instr 1
ichannel = p1
itime = p2
iduration = p3
ikey = p4
ivelocity = p5
print itime, iduration, ichannel, ikey, ivelocity
jacknoteout "noteout", "aeolus:Midi/in", ichannel, ikey, ivelocity
endin

instr jackin
aright jackaudioin "aeolus:out.L", "leftin"
aleft jackaudioin "aeolus:out.R", "rightin"
outs  aright, aleft
endin

</CsInstruments>
<CsScore>
i 1 1 30 60 60
i 1 2 30 64 60
i 1 3 30 71 60
</CsScore>
</CsoundSynthesizer>

