<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o hvs2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr=48000
ksmps=100
nchnls=2

; Example by James Hearon 2008
; Edited by Andres Cabrera

ginumPointsX init       16
ginumPointsY init       16
ginumParms  init        3

;Generate 9 tables with arbitrary points
gitmp ftgen 100, 0, 16, -2, 70, 260, 390, 180, 200, 300, 980, 126, \
                330, 860, 580, 467, 220, 399, 1026, 1500
gitmp ftgen 200, 0, 16, -2, 100, 200, 300, 140, 600, 700, 880, 126, \
                330, 560, 780, 167, 220, 999, 1026, 1500
gitmp ftgen 300, 0, 16, -2, 400, 200, 300, 540, 600, 700, 880, 126, \
                330, 160, 780, 167, 820, 999, 1026, 1500
gitmp ftgen 400, 0, 16, -2, 100, 200, 800, 640, 600, 300, 880, 126, \
                330, 660, 780, 167, 220, 999, 1026, 1500
gitmp ftgen 500, 0, 16, -2, 200, 200, 360, 440, 600, 700, 880, 126, \
                330, 560, 380, 167, 220, 499, 1026, 1500
gitmp ftgen 600, 0, 16, -2, 100, 600, 300, 840, 600, 700, 880, 126, \
                330, 260, 980, 367, 120, 399, 1026, 1500
gitmp ftgen 700, 0, 16, -2, 100, 200, 300, 340, 200, 500, 380, 126, \
                330, 860, 780, 867, 120, 999, 1026, 1500
gitmp ftgen 800, 0, 16, -2, 100, 600, 300, 240, 200, 700, 880, 126, \
                130, 560, 980, 167, 220, 499, 1026, 1500
gitmp ftgen 900, 0, 16, -2, 100, 800, 200, 140, 600, 700, 680, 126, \
                330, 560, 780, 167, 120, 299, 1026, 1500

giOutTab ftgen   5,0,8, -2,      0
giPosTab ftgen   6,0,32, -2, 0,1,2,3,4,5,6,7,8,9,10, 11, 15, 14, 13, 12
giSnapTab ftgen   8,0,64, -2,  1,1,1,   2,0,0,  3,2,0,  2,2,2,  \
        5,2,1,  2,3,4,  6,1,7,  0,0,0,  1,3,5,  3,4,4,  1,5,8,  1,1,5,  \
        4,3,2,  3,4,5,  7,6,5,  7,8,9

tb0_init        giOutTab

        FLpanel "hsv2",440,100,10,10,0
gk1,ih1 FLslider "X", 0,1, 0, 5, -1, 400,20, 20,10
gk2, ih2 FLslider "Y", 0, 1, 0, 5, -1, 400, 20, 20, 50
        FLpanel_end

        FLpanel "hvsBox",280,280,500,1000,0
;ihandle FLhvsBox inumlinesX, inumlinesY, iwidth, iheight, ix, iy [, image]
gih1  FLhvsBox  16, 16, 250, 250, 10, 1
        FLpanel_end
        FLrun


        instr   1
FLhvsBoxSetValue gk1, gk2, gih1

hvs2    gk1,gk2,  ginumParms, ginumPointsX, ginumPointsY, giOutTab, giPosTab, giSnapTab  ;, iConfigTab

k0      init    0
k1      init    1
k2      init    2
kspeed  init    0

kspeed = int((tb0(k2)) + 1)*.10

kenv  oscil   25000, kspeed*16, 10 

k1    phasor kspeed ;slow phasor: 200 sec.
kpch  tableikt k1 * 16, int((tb0(k1)) +1)*100 ;scale phasor * length
a1    oscilikt kenv, kpch, int(tb0(k0)) +1000;scale pitch slightly
ahp butterlp a1, 2500
outs ahp, ahp

        endin


</CsInstruments>
<CsScore>

f 10  0  1024 20  5 ;use of windowing function
f1000 0 1024 10 .33 .25 .5
f1001 0 1024 10 1
f1002 0 1024 10 .5  .25  .05
f1003 0 1024 10 .05  .10  .3  .5  1
f1004 0 1024 10 1 .5  .25  .125  .625
f1005 0 1024 10 .33  .44  .55  .66
f1006 0 1024 10 1 1 1 1 1
f1007 0 1024 10 .05 .25 .05 .25 .05 1

f0 3600
i1 0 3600

</CsScore>
</CsoundSynthesizer>