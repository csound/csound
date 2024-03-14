<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o chuap.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

gibuzztable ftgen 1, 0, 16384, 10, 1

instr 1	
	
    istep_size    =       p4
    iL            =       p5
    iR0           =       p6
    iC2           =       p7
    iG            =       p8
    iGa           =       p9
    iGb           =       p10
    iE            =       p11
    iC1           =       p12
    iI3           =       p13
    iV2           =       p14
    iV1           =       p15

    iattack       =       0.02
    isustain      =       p3
    irelease      =       0.02
    p3            =       iattack + isustain + irelease
    iscale        =       1.0
    adamping      linseg  0.0, iattack, iscale, isustain, iscale, irelease, 0.0
    aguide        buzz    0.5, 440, sr/440, gibuzztable
    aI3, aV2, aV1 chuap   iL, iR0, iC2, iG, iGa, iGb, iE, iC1, iI3, iV2, iV1, istep_size 
    asignal       balance aV2, aguide

    outs asignal*adamping, asignal*adamping
endin

</CsInstruments>
<CsScore> 
;        Adapted from ABC++ MATLAB example data.
//             time_step      kL           kR0         kC2              kG            kGa           kGb          kE          kC1           iI3                     iV2                     iV1
; torus attractor ( gallery of attractors ) 
i 1 0 20       .1            -0.00707925   0.00001647  100              1             -.99955324    -1.00028375  1          -.00222159     -2.36201596260071       3.08917625807226e-03    3.87075614929199   
; heteroclinic orbit
i 1 + 20       .425           1.3506168    0           -4.50746268737  -1             2.4924        .93          1           1             -22.28662665            .009506608              -22.2861576            
; periodic attractor (torus breakdown route)
i 1 + 20       .05            0.00667      0.000651    10              -1             .856          1.1          1           .06           -20.200590133667        .172539323568344        -4.07686233520508      
; torus attractor (torus breakdown route)'
i 1 + 20       0.05           0.00667      0.000651    10              -1             0.856         1.1          1           0.1            21.12496758             0.03001749              0.515828669            

</CsScore>
</CsoundSynthesizer>
