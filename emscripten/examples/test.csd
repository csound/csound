<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
nchnls = 2
0dbfs = 1
ksmps = 256
sr = 44100

instr VCO2

    aVar oscili 0.2, 100, 1
    outs aVar, aVar
endin


</CsInstruments>
<CsScore>
f1 0 128 10 1
i "VCO2" 0 1

</CsScore>
</CsoundSynthesizer>
