<CsoundSynthesizer>
<CsOptions>
-+rtaudio=jack -odac -b4096
</CsOptions>
<CsInstruments>
sr     =     44100
ksmps     =     32
nchnls     =     2
0dbfs    =    1

 opcode SimpleTest, k, a
     setksmps 1
     aInput xin
 endop


instr 1
a1 init 0
k1 SimpleTest a1
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>