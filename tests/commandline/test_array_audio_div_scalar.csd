<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode assert,0,kk
k1, k2 xin
if k1 != k2 then
 printks "assert error\n", 1
 exitnowk(-1)
endif
endop

instr 1
 aNumerator[] init 1
 aNumerator[0] = 8
 aDenominator = 2

 aResult[] = aNumerator / aDenominator
 assert(k(aResult[0]), 4)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
