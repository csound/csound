<CsoundSynthesizer>
<CsOptions>
-odac -iadc -d
</CsOptions>

<CsInstruments>
/*************************************/
/* Independent time & pitch scaler   */
/* using syncgrain                   */
/* Victor Lazzarini, 2005            */
/*************************************/
sr = 44100
0dbfs=1
ksmps = 64
nchnls = 2

FLcolor  200,10,40, 140,60,60

        FLpanel "TPScaler",220,435,200,200
FLcolor  140,10,40, 140,60,60
iv1 FLvalue  "time ratio",  80, 20, 20, 240
iv2 FLvalue  "pitch ratio", 80, 20, 110, 240
FLcolor  140,60,60, 200,10,40
gk1,gk2, ih1, ih2 FLjoy  "timescale x pitchscale",-2,2,-2,2,0,0,iv1,iv2, 200,200,10,10
FLcolor  200,10,40, 140,60,60
gk3, ih3  FLslider  "grain size (0.01-0.5 secs)", 0.01, 0.5,0,5,-1,200,20,10,295
gk4, ih4  FLslider  "overlapped grains (1 - 50)", 1,50,0,5,-1,200,20,10,340
gk5, ih5  FLslider  "amplitude", 0.1,1,-1,5,-1,200,20,10,385

FLsetVal_i 1, ih1
FLsetVal_i 1, ih2
FLsetVal_i 0.04, ih3
FLsetVal_i 2, ih4
FLsetVal_i 0.7, ih5

        FLpanelEnd
        FLrun

instr 1

iomax = 100          /* just to be in the safe side */
kol = int(gk4)       /* number of overlapped grains */
kgr = gk3            /* grain size in secs */
kfr = kol/kgr        /* freq or density in gr/sec */
kps = 1/kol          /* pointer rate scaling */

/* gk1 controls timescale, gk2 controls pitchscale */

ain inch 1
awp  phasor sr/ftlen(1)
awin tablei awp, 2, 1
     tablew ain*awin, awp, 1, 1
asig syncgrain  gk5, kfr, gk2, kgr, kps*gk1,1, 3, iomax

   outch    1, asig

endin

</CsInstruments>

<CsScore>
f2 0 131072 7 0 36 1 131000 1 36 0
f3 0  16384 9 0.5 1 0
f1 0 131072 7 0 131072 0
i1 0 3600

</CsScore>

</CsoundSynthesizer>
