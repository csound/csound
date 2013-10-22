<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>

sr= 44100
kr = 4410
ksmps = 10
nchnls= 4

instr 1
kfreq chnget "freq"
kamp  chnget"amp"
a0    = 0
asyn  oscili p4*kamp, p5*kfreq, 1
        outq asyn*0.7, asyn*0.3,a0,a0

S1 chnget "blar"
printf "blar: %s, amp: %f freq: %f \n", kamp, S1, kamp, kfreq*p5

endin

instr 2

as1,as2,as3,as4 inq

krvt  invalue "rev"
klt   invalue "del"
kfm   invalue "modf"
kmod  phasor kfm
adel  upsamp klt

a0  = 0
acmb flanger  as4, adel, krvt
      outvalue "mod",  kmod
      outq a0,a0,as4*0.1, acmb*0.1
endin

</CsInstruments>

<CsScore>
f0 450000
f1 0 4096 10 1
i2 0 450000
</CsScore>

</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>0</width>
 <height>0</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
