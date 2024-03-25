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
gScurves[] fillarray "ELLIPSE ", "LEMNISKATE ", "LIMACON ","CORNOID ","TRISEXTIC ","SCARABEUS ","FOLIUM ","TALBOT "

instr 1 
prints gScurves[p4]
kdclk init 0
kdclk   linsegr  0, 0.1, 1, p3-0.02, 1, 0.9, 0
kparam init 0
krot init 0
krot linseg 0,p3,6.28 ; ROTATE THE CURVE SLOWLY
kparam linseg 0,p3,4 ; INCREASE CURVEPARAMETER

asigL wterrain2 0.5, 110, 0.5, 0.5, 0.8, 0.4, krot, gisine, gisine, p4, kparam
; LET THE POINT FOR RIGHT RUNNING BACKWARDS FOR A LITTLE STEREO EFFECT
asigR wterrain2 0.5, -110, 0.5, 0.5, 0.8, 0.4, krot, gisine, gisine, p4, kparam
asigR dcblock asigL
asigL dcblock asigR
asigL*=kdclk
asigR*=kdclk
aL,aR reverbsc asigL, asigR, 0.6, 15000, 44100, 0.5, 1
outs aL,aR

endin

</CsInstruments>

<CsScore>

i 1 0 40 0 ; ELLIPSE
i 1 41 40 1; LEMNSIKATE
i 1 82 40 2; LIMACON
i 1 123 40 3; CORNOID
i 1 164 40 4; TRISEXTIC
i 1 205 40 5; SCARABEUS
i 1 246 40 6; FOLIUM
i 1 287 40 7; TALBOT

e
</CsScore>
</CsoundSynthesizer>

