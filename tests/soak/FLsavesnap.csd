<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
</CsOptions>
<CsInstruments>

sr=48000
ksmps=128
nchnls=2

; Example by Hector Centeno and Andres Cabrera 2007

; giSWMtab4 ftgen 0, 0, 513, 21, 10, 1, .3
; giSWMtab4M ftgen 0, 0, 64, 7, 1, 50, 1

FLpanel	"Snapshots", 530, 350, 40, 410, 3
    FLcolor  100, 118 ,140
   FLsetSnapGroup  0
    ivalSM1		 	FLvalue  "", 70, 20, 270, 20
    ivalSM2			FLvalue  "", 70, 20, 270, 60
    ivalSM3			FLvalue  "", 70, 20, 270, 100
    ivalSM4			FLvalue  "", 70, 20, 270, 140
    gksliderA, gislidSM1 	FLslider "Slider A", -4, 4, 0, 3, ivalSM1, 250, 20, 20, 20
    gksliderB, gislidSM2 	FLslider "Slider B", 1, 10, 0, 3, ivalSM2, 250, 20, 20, 60
    gksliderC, gislidSM3 	FLslider "Slider C", 0, 1, 0, 3, ivalSM3, 250, 20, 20, 100
    gksliderD, gislidSM4 	FLslider "Slider D", 0, 1, 0, 3, ivalSM4, 250, 20, 20, 140
    itext1		 FLbox	  "store", 1, 1, 14, 50, 25, 355, 15
    itext2		 FLbox	  "load", 1, 1, 14, 50, 25, 415, 15
    itext3		 FLbox	  "Group 1", 1, 1, 14, 30, 145, 485, 15
    gksnap, ibuttn1	 FLbutton "1", 1, 0, 11, 25, 25, 364, 45, 0, 3, 0, 3, 1
    gksnap, ibuttn2	 FLbutton "2", 1, 0, 11, 25, 25, 364, 75, 0, 3, 0, 3, 2
    gksnap, ibuttn3	 FLbutton "3", 1, 0, 11, 25, 25, 364, 105, 0, 3, 0, 3, 3
    gksnap, ibuttn4	 FLbutton "4", 1, 0, 11, 25, 25, 364, 135, 0, 3, 0, 3, 4
    gkload, ibuttn1	 FLbutton "1", 1, 0, 11, 25, 25, 424, 45, 0, 4, 0, 3, 1
    gkload, ibuttn2	 FLbutton "2", 1, 0, 11, 25, 25, 424, 75, 0, 4, 0, 3, 2
    gkload, ibuttn3	 FLbutton "3", 1, 0, 11, 25, 25, 424, 105, 0, 4, 0, 3, 3
    gkload, ibuttn4	 FLbutton "4", 1, 0, 11, 25, 25, 424, 135, 0, 4, 0, 3, 4

    FLcolor  100, 140 ,118
   FLsetSnapGroup  1
    ivalSM5		 	FLvalue  "", 70, 20, 270, 190
    ivalSM6			FLvalue  "", 70, 20, 270, 230
    ivalSM7			FLvalue  "", 70, 20, 270, 270
    ivalSM8			FLvalue  "", 70, 20, 270, 310
    gkknobA, gislidSM5 	FLknob "Knob A", -4, 4, 0, 3, ivalSM5, 45, 10, 230
    gkknobB, gislidSM6 	FLknob "Knob B", 1, 10, 0, 3, ivalSM6, 45, 75, 230
    gkknobC, gislidSM7 	FLknob "Knob C", 0, 1, 0, 3, ivalSM7, 45, 140, 230
    gkknobD, gislidSM8 	FLknob "Knob D", 0, 1, 0, 3, ivalSM8, 45, 205, 230
    itext4		 FLbox	  "store", 1, 1, 14, 50, 25, 355, 185
    itext5		 FLbox	  "load", 1, 1, 14, 50, 25, 415, 185
    itext6		 FLbox	  "Group 2", 1, 1, 14, 30, 145, 485, 185
    gksnap, ibuttn1	 FLbutton "5", 1, 0, 11, 25, 25, 364, 215, 0, 3, 0, 3, 5
    gksnap, ibuttn2	 FLbutton "6", 1, 0, 11, 25, 25, 364, 245, 0, 3, 0, 3, 6
    gksnap, ibuttn3	 FLbutton "7", 1, 0, 11, 25, 25, 364, 275, 0, 3, 0, 3, 7
    gksnap, ibuttn4	 FLbutton "8", 1, 0, 11, 25, 25, 364, 305, 0, 3, 0, 3, 8
    gkload, ibuttn1	 FLbutton "5", 1, 0, 11, 25, 25, 424, 215, 0, 4, 0, 3, 5
    gkload, ibuttn2	 FLbutton "6", 1, 0, 11, 25, 25, 424, 245, 0, 4, 0, 3, 6
    gkload, ibuttn3	 FLbutton "7", 1, 0, 11, 25, 25, 424, 275, 0, 4, 0, 3, 7
    gkload, ibuttn4	 FLbutton "8", 1, 0, 11, 25, 25, 424, 305, 0, 4, 0, 3, 8
FLpanelEnd
FLsetVal_i 1, gislidSM1
FLsetVal_i 1, gislidSM2
FLsetVal_i 0, gislidSM3
FLsetVal_i 0, gislidSM4
FLsetVal_i 1, gislidSM5
FLsetVal_i 1, gislidSM6
FLsetVal_i 0, gislidSM7
FLsetVal_i 0, gislidSM8
FLrun

	instr 1

	endin

instr 3 ; Save snapshot
index init 0
ipstno = p4
igroup = 0
Sfile sprintf "PVCsynth.%d.snap", ipstno 
if ipstno > 4 then
  igroup = 1
endif


  inumsnap, inumval  FLsetsnap index , -1, igroup
FLsavesnap Sfile

	endin


instr 4 ;Load snapshot
index init 0
ipstno = p4
igroup = 0
Sfile sprintf "PVCsynth.%d.snap", ipstno
if ipstno > 4 then
  igroup = 1
endif

FLloadsnap Sfile
  inumload FLgetsnap index , igroup

	endin


</CsInstruments>

<CsScore>
  ;Dummy table for FLgetsnap
; f 1 0 1024 10 1
f 0 3600

e

</CsScore>

</CsoundSynthesizer>
