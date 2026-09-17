<CsTest>
description = "Zak rejects unrepresentable indices and signed modulation overflow"
[expect]
exit = "nonzero"
stderr = ["Not writing."]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
zakinit 4, 4
instr 1
iValue zir 1e30
endin
instr 2
kValue zkr 1e30
endin
instr 3
ziw 1, 1e30
endin
instr 4
zkw 1, 1e30
endin
instr 5
ziwm 1, 1e30
endin
instr 6
zkwm 1, 1e30
endin
instr 7
kValue zkmod 1, -2147483648
endin
instr 8
zkcl 0, 1e30
endin
instr 9
aValue zar 1e30
endin
instr 10
aValue zarg 1e30, 1
endin
instr 11
aValue = 1
zaw aValue, 1e30
endin
instr 12
aValue = 1
zawm aValue, 1e30
endin
instr 13
aInput = 1
aValue zamod aInput, -2147483648
endin
instr 14
zacl 0, 1e30
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01
i 4 0 .01
i 5 0 .01
i 6 0 .01
i 7 0 .01
i 8 0 .01
i 9 0 .01
i 10 0 .01
i 11 0 .01
i 12 0 .01
i 13 0 .01
i 14 0 .01
e
</CsScore>
</CsoundSynthesizer>
