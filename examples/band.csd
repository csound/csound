<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
/* pvs examples with sliding
*/
sr     = 44100
ksmps  = 100
nchnls = 2

instr  1
       kl   line       70, p3, 10000
       asig oscil      16000, kl, 1 
       fsig pvsanal    asig,128,1,128,1
       fs   pvsband    fsig, 100, 200, 1000, 9000
       acln pvsynth    fs
            outs       acln, asig
endin
</CsInstruments>

<CsScore>
f1 0 4096 10 1
i1 0 6
e
</CsScore>
</CsoundSynthesizer>