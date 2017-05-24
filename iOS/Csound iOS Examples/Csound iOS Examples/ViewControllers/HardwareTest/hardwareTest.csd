<CsoundSynthesizer>
<CsOptions>
-o dac
-d
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 64
nchnls    = 2
0dbfs	  = 1

instr 1

kaccelX chnget "accelerometerX" 
kaccelY chnget "accelerometerY" 
kaccelZ chnget "accelerometerZ" 

kgyroX chnget "gyroX" 
kgyroY chnget "gyroY" 
kgyroZ chnget "gyroZ" 

kattRoll chnget "attitudeRoll" 
kattPitch chnget "attitudePitch" 
kattYaw chnget "attitudeYaw" 


kcutoff = 5000 + (4000 * kattYaw)
kresonance = .3 + (.3  * kattRoll)
kpch = 880 + (kaccelX * 220)

a1 vco2 (kattPitch + .5)  * .2, kpch

a1 moogladder a1, kcutoff, kresonance

aL, aR reverbsc a1, a1, .72, 5000

outs aL, aR


endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i1 0 360000
 
</CsScore>
</CsoundSynthesizer>
