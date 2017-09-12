<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o chuas_oscillator.wav.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

gibuzztable ftgen 1, 0, 16384, 10, 1

instr 1
; sys_variables = system_vars(5:12); % L,R0,C2,G,Ga,Gb,E,C1 or p8:p15
; integ_variables = [system_vars(14:16),system_vars(1:2)]; % x0,y0,z0,dataset_size,step_size or p17:p19, p4:p5
istep_size    =       p5
iL            =       p8
iR0           =       p9
iC2           =       p10
iG            =       p11
iGa           =       p12
iGb           =       p13
iE            =       p14
iC1           =       p15
iI3           =       p17
iV2           =       p18
iV1           =       p19
iattack       =       0.02
isustain      =       p3
irelease      =       0.02
p3            =       iattack + isustain + irelease
iscale        =       1.0
adamping      linseg  0.0, iattack, iscale, isustain, iscale, irelease, 0.0
aguide        buzz    0.5, 440, sr/440, gibuzztable
aI3, aV2, aV1 chuap   iL, iR0, iC2, iG, iGa, iGb, iE, iC1, iI3, iV2, iV1, istep_size 
asignal       balance aV2, aguide
              outs    adamping * asignal, adamping * asignal
endin
</CsInstruments>
<CsScore>
;        Adapted from ABC++ MATLAB example data.
i 1 0 20 1500 .1   -1 -1 -0.00707925 0.00001647 100  1 -.99955324 -1.00028375 1 -.00222159 204.8 -2.36201596260071 3.08917625807226e-03 3.87075614929199 7 .4 .004 1 86 30; torus attractor ( gallery of attractors ) 
i 1 + 20 1500 .425  0 -1  1.3506168  0              -4.50746268737 -1 2.4924 .93 1 1 0 -22.28662665 .009506608 -22.2861576 32 10 2 20 86 30 ; heteroclinic orbit
i 1 + 20 1024 .05  -1 -1  0.00667    0.000651    10 -1 .856 1.1 1 .06 51.2 -20.200590133667 .172539323568344 -4.07686233520508 2.5 10 .2 1 66 81 ; periodic attractor (torus breakdown route)
i 1 + 20 1024 0.05 -1 -1 0.00667 0.000651 10 -1 0.856 1.1 1 0.1 153.6 21.12496758 0.03001749 0.515828669 2.5 10 0.2 1 66 81  ; torus attractor (torus breakdown route)'
</CsScore>
</CsoundSynthesizer>
