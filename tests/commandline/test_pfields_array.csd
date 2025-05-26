<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1


instr 1
ipfields[] passign 4
iend = lenarray(ipfields) - 1
print ipfields[iend]
print p5000
i1 ftgen 0, 0, 0, 2, ipfields ; tests arrays for ftgen
turnoff
endin


instr 2
ipfields[] genarray 1, 5000
schedule ipfields
turnoff
endin


</CsInstruments>
<CsScore>
i 2 0 3
</CsScore>
</CsoundSynthesizer>