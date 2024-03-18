<CsoundSynthesizer>

<CsOptions>
  -d -o dac
</CsOptions>

<CsInstruments>
sr     = 48000
ksmps  = 10
nchnls = 2
0dbfs  = 1

          instr 1
idur       =          p3 
iamp       =          p4 
ifrq1      =          p5 
ifrq2      =          p6 
indx1      =          p7 
indx2      =          p8 
kenv       linen      iamp, 0.05, idur, 0.2 
a1, a2     crossfm    ifrq1, ifrq2, indx1, indx2, 1, 1, 1 
           outs       a1*kenv, a2*kenv 
          endin

          instr 2
kx         init       0.0 
kdx        init       0.1 
kdur       init       p4 
kamp       init       p5 
kndx       init       0 
kmax       init       p6 
kfq1       init       440.0 
kfq2       init       557.0 
knx1       init       3.5 
knx2       init       4.8 
           event      "i", 1, kx, kdur, kamp, kfq1, kfq2, knx1, knx2 
kx         =          kx + kdx 
knx1       =          knx1 - 0.025 
knx2       =          knx2 - 0.075 
kndx       =          kndx + 1 
if (kndx <= kmax) kgoto nextone
           turnoff 
nextone: 
          endin
</CsInstruments>

<CsScore>
f 1 0 16384 10 1 0

i 2 0 2 0.67 0.16 150
i 1 15.1 1.5 0.1 440 557 -0.25 -6.45 0.05 1.2
e
</CsScore>
</CsoundSynthesizer>

