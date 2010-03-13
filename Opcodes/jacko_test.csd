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
;jackfreewheel 0
jackaudioinconnect "aeolus:out.L", "leftin"
jackaudioinconnect "aeolus:out.R", "rightin"
jackmidioutconnect "midiout", "aeolus:Midi/in"
jackinfo
jackon

alwayson "jackin"

instr 1
ichannel = p1
itime = p2
iduration = p3
ikey = p4
ivelocity = p5
jacknoteout "midiout", ichannel, ikey, ivelocity
print itime, iduration, ichannel, ikey, ivelocity
endin

instr jackin
aright jackaudioin "leftin"
aleft jackaudioin "rightin"
outs  aright, aleft
endin

</CsInstruments>
<CsScore>
i 1 1 30 60 60
i 1 2 30 64 60
i 1 3 30 71 60
e 2
</CsScore>
</CsoundSynthesizer>

