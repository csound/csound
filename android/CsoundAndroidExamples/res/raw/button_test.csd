<CsoundSynthesizer>
<CsOptions>
-o dac -d -+msg_color=0 -m0 -b512 -B1024

</CsOptions>
<CsInstruments>
nchnls=2
0dbfs=1
ksmps=64
sr = 44100

seed 0

instr 1
;a1 inch 1
 ;  outs a1, a1


ktrigger chnget "button1" 
knotedur chnget "duration" 

printk2 ktrigger
if(ktrigger > 0) then
    event "i", 2, 0, knotedur
    ktrigger = 0;
    chnset ktrigger, "button1"
endif

endin

instr 2

;print p3

iattack chnget "attack" 
idecay chnget "decay" 
isustain chnget "sustain" 
irelease chnget "release" 

;ipchMul rnd31 .5, -0.5
;ipchMul = ipchMul + .5
;ipch = 100 + (1000 * ipchMul)
ipch = 100 + rnd(1000)

;print iattack
;print idecay
;print isustain
;print irelease

a2 linsegr 0, iattack, 1, idecay, isustain, irelease, 0
a1 oscili a2, ipch, 1
outs a1*0.1,a1*0.1
endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i1 0 3600
 
</CsScore>
</CsoundSynthesizer>
