<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
nchnls = 2
0dbfs = 1
ksmps = 256
sr = 44100

schedule 1, 0, -1

instr 1
    klfo lfo 1, 1
    iport init 8888
    kinput websocket iport, klfo
    printk2 kinput
endin

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>
