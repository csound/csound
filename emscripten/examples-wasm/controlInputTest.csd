<CsoundSynthesizer>
<CsOptions>
-o dac -+rtmidi=null -+rtaudio=null -d -+msg_color=0 -M0 -m0
</CsOptions>
<CsInstruments>
nchnls = 2
0dbfs = 1

instr VCO2

    kValue chnget "channelName"
    aVar vco2 0.2, 220 + kValue
    outs aVar, aVar
endin


</CsInstruments>
<CsScore>
i "VCO2" 0 10

</CsScore>
</CsoundSynthesizer>
