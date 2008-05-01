<CsoundSynthesizer>
<CsOptions>
-RWfo chuas_oscillator.wav
</CsOptions>

<CsInstruments>

sr     = 44100
ksmps  = 100
nchnls = 2
0dbfs  = 1000

instr  1
; sys_variables = system_vars(5:12); % L,R0,C2,G,Ga,Gb,E,C1 or p8:p15
; integ_variables = [system_vars(14:16),system_vars(1:2)]; % x0,y0,z0,dataset_size,step_size or p17:p19, p4:p5
istep_size = p5
iL = p8
iR0 = p9
iC2 = p10
iG = p11
iGa = p12
iGb = p13
iE = p14
iC1 = p15
iI3 = p17
iV2 = p18
iV1 = p19
iattack = 0.02
isustain = p3
irelease = 0.02
p3 = iattack + isustain + irelease
adeclick linseg 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
aI3, aV2, aV1 chuap iL, iR0, iC2, iG, iGa, iGb, iE, iC1, iI3, iV2, iV1, istep_size	
outs adeclick * aV2, adeclick * aV2
endin


</CsInstruments>

<CsScore>
i 1 0 20 1500 .1 -1 -1 -.00707925 .00001647 100 1 -.99955324 -1.00028375 1 -.00222159 204.8 -2.36201596260071 3.08917625807226e-03 3.87075614929199 7 .4 .004 1 86 30; torus attractor ( gallery of attractors ) 
i 1 + 20 1500 .425 0 -1 1.3506168 0 -4.50746268737 -1 2.4924 .93 1 1 0 -22.28662665 .009506608 -22.2861576 32 10 2 20 86 30 ; heteroclinic orbit
i 1 + 20 1024 .05 -1 -1 .00667 .000651 10 -1 .856 1.1 1 .06 51.2 -20.200590133667 .172539323568344 -4.07686233520508 2.5 10 .2 1 66 81 ; periodic attractor (torus breakdown route)
</CsScore>
</CsoundSynthesizer>