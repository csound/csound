<CsoundSynthesizer>
<CsOptions>
csound -odac4 temp.orc temp.sco
</CsOptions>
<CsInstruments>
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; gihandle1 vstinit "c:/projects/music/library/polyiblit.dll", 1
gihandle1 vstinit "c:/Program Files/Steinberg/Cubase SX/vstplugins/Synths/Cheeze Machine/Cheeze Machine.dll", 1
vstedit gihandle1

instr 1
ichannel = p1 - 1
ikey = p4
ivelocity = p5
vstnote gihandle1, ichannel, ikey, ivelocity, p3
ain1 = 0
ab1, ab2 vstplug gihandle1, ain1, ain1
outs ab1, ab2
endin
</CsInstruments>
<CsScore>
i 1 0 10 60 60
i 1 2 1 72 60
i 1 2 2 64 60
i 1 15 15 60.375 60
i 1 15 15 64 60
i 1 18 15 36 60
i 1 15 400 62 60
</CsScore>
</CsoundSynthesizer>
