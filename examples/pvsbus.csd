<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
/* pvs bus interface example, works with pvsbus.c
   Copies a fsig to the pvs out bus and retrieves
   it in the pvs in bus.
*/
sr = 44100
ksmps=100
nchnls=2

instr 1                                                                               
asig oscili 16000, 100,1 
fsig pvsanal  asig,1024,256,1024,1               

     pvsout fsig, 0
fbus pvsin  0                                     ;

aleft  pvsynth fbus
aright pvsynth fsig
 outs aleft,aright     

endin
</CsInstruments>

<CsScore>
f1 0 1024 10 1
i1 0 10
</CsScore>
</CsoundSynthesizer>