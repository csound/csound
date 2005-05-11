<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>

sr= 44100
kr = 4410
ksmps = 10
nchnls= 2

instr 1
kfreq invalue "freq"
kamp  invalue "amp"
asyn  oscili p4*kamp, p5*kfreq, 1  
        outs asyn*0.7, asyn*0.3
endin

instr 2

as1,as2 ins
krvt  invalue "rev"
klt   invalue "del"
kfm   invalue "modf"
kmod  phasor kfm
adel upsamp klt
acmb flanger  as1, adel, krvt
      outvalue "mod",  kmod
        outs acmb*0.3, acmb*0.7
endin


</CsInstruments>

<CsScore>
f0 36000
f1 0 4096 10 1 
i2 0 3600 
</CsScore>

</CsoundSynthesizer>
