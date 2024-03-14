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

FLpanel	"Snapshots", 530, 190, 40, 410, 3
    FLcolor  100, 118 ,140
    ivalSM1		 	FLvalue  "", 70, 20, 270, 20
    gksliderA, gislidSM1 	FLslider "Slider", -4, 4, 0, 3, ivalSM1, 250, 20, 20, 20
    itext1		 FLbox	  "store", 1, 1, 14, 50, 25, 355, 15
    itext2		 FLbox	  "load", 1, 1, 14, 50, 25, 415, 15
    gksnap, ibuttn1	 FLbutton "1", 1, 0, 11, 25, 25, 364, 45, 0, 3, 0, 3, 1
    gksnap, ibuttn2	 FLbutton "2", 1, 0, 11, 25, 25, 364, 75, 0, 3, 0, 3, 2
    gksnap, ibuttn3	 FLbutton "3", 1, 0, 11, 25, 25, 364, 105, 0, 3, 0, 3, 3
    gksnap, ibuttn4	 FLbutton "4", 1, 0, 11, 25, 25, 364, 135, 0, 3, 0, 3, 4

    gkload, ibuttn1	 FLbutton "1", 1, 0, 11, 25, 25, 424, 45, 0, 4, 0, 3, 1
    gkload, ibuttn2	 FLbutton "2", 1, 0, 11, 25, 25, 424, 75, 0, 4, 0, 3, 2
    gkload, ibuttn3	 FLbutton "3", 1, 0, 11, 25, 25, 424, 105, 0, 4, 0, 3, 3
    gkload, ibuttn4	 FLbutton "4", 1, 0, 11, 25, 25, 424, 135, 0, 4, 0, 3, 4

    ivalSM2		 	FLvalue  "", 70, 20, 270, 80
    gkknobA, gislidSM2 	FLknob "Knob", -4, 4, 0, 3, ivalSM2, 60, 120, 60
FLpanelEnd
FLsetVal_i 1, gislidSM1
FLsetVal_i 1, gislidSM2
FLrun

	instr 1

	endin

instr 3 ; Save snapshot
index init 0
ipstno = p4
Sfile sprintf "snapshot_simple.%d.snap", ipstno 


inumsnap, inumval  FLsetsnap index ;, -1, igroup
FLsavesnap Sfile

	endin


instr 4 ;Load snapshot
index init 0
ipstno = p4
Sfile sprintf "snapshot_simple.%d.snap", ipstno

FLloadsnap Sfile
inumload FLgetsnap index ;, igroup

	endin


</CsInstruments>

<CsScore>
f 0 3600

e

</CsScore>

</CsoundSynthesizer>
