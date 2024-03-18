<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>
sr     = 48000
ksmps  = 32
nchnls = 2
0dbfs  = 1

gisine ftgen 0,0,2^12,10,1

instr 1

kamp adsr 0.1,0.01,1,10

; THE MODULATION PARAMETERS 
krot linseg 0,p3,6.28 ; ROTATE
kparam randc 2.5,0.1,0.993 ; CONTROL CURVEPARAMETER WITH CUBIC RANDOM 
kx linseg 0,p3/2,1,p3/2,0 ; MOVE HORIZONTAL THROUGH THE TERRAIN
ky linseg 0,p3,1 ; MOVE VERTICAL THROUGH THE TERRAIN

; TRY THE OTHER CURVES AS WELL 
asigL wterrain2 0.2, 55*p4, kx, ky, 0.5, 0.4, krot, gisine, gisine, 5, kparam
asigR wterrain2 0.2, -55*p4, kx, ky, 0.5, 0.4, krot, gisine, gisine, 5, kparam

asigR dcblock asigL
asigL dcblock asigR
asigL butterlp asigL,9000
asigR butterlp asigR,9000
aL,aR reverbsc asigL, asigR, 0.7, 15000, 44100, 0.5, 1
outs aL*kamp*p5,aR*kamp*p5

endin

</CsInstruments>

<CsScore>

i 1 0 300 1 1
i 1 1 300 7 0.4
i 1 2 299 [9/2] 0.4
i 1 3.1 296.9 [16/3] 0.45
i 1 4.3 295.7 [27/4] 0.5
i 1 6.9 292.1 6 0.5 

e
</CsScore>
</CsoundSynthesizer>

