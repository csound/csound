<CsoundSynthesizer>

<CsInstruments>
nchnls = 2
0dbfs = 1
instr 1
outch 1, rand:a(.1)
endin
schedule(1,0,5)
instr 2
aOut[] monitor
printk 1, k(aOut[0])
printk 1, k(aOut[1])
endin
schedule(2,0,5)


</CsInstruments>

<CsScore>
fo 10
e
</CsScore>

</CsoundSynthesizer>
