<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>

sr= 44100
kr = 4410
ksmps = 10
nchnls= 4

instr 1
kfreq invalue "freq"
kamp  invalue "amp"
a0    = 0
asyn  oscili p4*kamp, p5*kfreq, 1
        outq asyn*0.7, asyn*0.3,a0,a0
endin

instr 2

as1,as2,as3,as4 inq
krvt  invalue "rev"
klt   invalue "del"
kfm   invalue "modf"
kmod  phasor kfm
adel upsamp klt
a0  = 0
acmb flanger  as4, adel, krvt
      outvalue "mod",  kmod
        outs a0,a0,acmb*0.3, acmb*0.7
endin

</CsInstruments>

<CsScore>
f0 450000
f1 0 4096 10 1
i2 0 450000
</CsScore>

</CsoundSynthesizer>
