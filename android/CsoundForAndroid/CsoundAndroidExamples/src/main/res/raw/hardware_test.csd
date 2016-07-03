<CsoundSynthesizer>
<CsOptions>
-o dac -+rtmidi=null -d -+msg_color=0 -M0 -m0
</CsOptions>
<CsInstruments>
nchnls=2
0dbfs=1
ksmps=64
sr = 44100

instr 1

kaccelX chnget "accelerometerX" 
kaccelY chnget "accelerometerY" 
kaccelZ chnget "accelerometerZ" 

;kgyroX chnget "gyroX" 
;kgyroY chnget "gyroY" 
;kgyroZ chnget "gyroZ" 

;kattRoll chnget "attitudeRoll" 
;kattPitch chnget "attitudePitch" 
;kattYaw chnget "attitudeYaw" 
kaccelX port kaccelX / 20, 0.01
kaccelY port kaccelY / 20, 0.01
kaccelZ port kaccelZ / 20, 0.01

kcutoff = 4000 + (2000 * kaccelX)
kresonance = .6 + (.3  * kaccelY)
kpch = 880 + kaccelX * 220

a1 vco2 (kaccelZ * .2) + .4, kpch

a1 moogladder a1, kcutoff, kresonance

aL, aR reverbsc a1, a1, .72, 5000

out aL, aR


endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i1 0 360000
 
</CsScore>
</CsoundSynthesizer>
