<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No display
-odac           -iadc     -d     ;;;RT audio I/O
</CsOptions>

<CsInstruments>

  sr	    =  44100
  ksmps	    =  10
  nchnls    =  1

; Example by Jonathan Murphy 2007

;;; reset amplitude range
  0dbfs	    =  1

;;; set the base colour for the panel
	    FLcolor   100, 0, 200
;;; define the panel
	    FLpanel   "FLexecButton", 250, 100, 0, 0
;;; sliders to control time stretch and pitch
  gkstr, gistretch    FLslider			  "Time", 0.5, 1.5, 0, 6, -1, 10, 60, 150, 20
  gkpch, gipitch      FLslider			  "Pitch", 0.5, 1.5, 0, 6, -1, 10, 60, 200, 20
;;; set FLexecButton colour
	    FLcolor   255, 255, 0
;;; when this button is pressed, fourier analysis is performed on the file
;;; "drumsMlp.wav", producing the analysis file "beats.pvx"
  gipvoc    FLexecButton  "csound -U pvanal drumsMlp.wav beats.pvx", 60, 20, 20, 20
;;; set FLexecButton text
	    FLsetText	"PVOC", gipvoc
;;; when this button is pressed, instr 10000 is called, exiting
;;; Csound immediately

;;; cancel previous  colour
	    FLcolor   -1
;;; set colour for kill button
	    FLcolor   255, 0, 0
  gkkill, gikill      FLbutton			  "X", 1, 1, 1, 20, 20, 100, 20, 0, 10000, 0, 0.1
;;; cancel previous colour
	    FLcolor   -1
;;; set colour for play/stop and pause buttons
	    FLcolor   0, 200, 0
;;; pause and play/stop buttons
  gkpause, gipause    FLbutton			  "@||", 1, 0, 2, 40, 20, 20, 60, -1
  gkplay, giplay      FLbutton			  "@|>", 1, 0, 2, 40, 20, 80, 60, -1
;;; end the panel
	    FLpanelEnd
;;; set initial values for time stretch and pitch
	    FLsetVal_i	1, gistretch
	    FLsetVal_i	1, gipitch
;;; run the panel
	    FLrun
    
	
    instr 1					  ; trigger play/stop
;;; is the play/stop button on or off?
;;; either way we need to trigger something,
;;; so we can't just use the value of gkplay
  kon	    trigger   gkplay, 0, 0
  koff	    trigger   gkplay, 1, 1
;;; if on, start instr 2
	    schedkwhen	kon, -1, -1, 2, 0, -1
;;; if off, stop instr 2
	    schedkwhen	koff, -1, -1, -2, 0, -1

    endin

    instr 2

;;; paused or playing?
if (gkpause == 1) kgoto pause
	    kgoto     start
pause:
;;; if the pause button is on, skip sound production
	    kgoto     end
start:
;;; get the length of the analysis file in seconds
  ilen	    filelen   "beats.pvx"
;;; determine base frequency of playback
  icps	    =  1/ilen
;;; create a table over the length of the file
  itpt	    ftgen     0, 0, 513, -7, 0, 512, ilen
;;; phasor for time control
  kphs	    phasor    icps * gkstr
;;; use phasor as index into table
  kndx	    =  kphs * 512
;;; read table 
  ktpt	    tablei    kndx, itpt 
;;; use value from table as time pointer into file
  fsig1	    pvsfread  ktpt, "beats.pvx"
;;; change playback pitch
  fsig2	    pvscale   fsig1, gkpch
;;; resynthesize
  aout	    pvsynth   fsig2
;;; envelope to avoid clicks and clipping
  aenv	    linsegr   0, 0.3, 0.75, 0.1, 0
  aout	    =  aout * aenv
	    out	      aout
end:

    endin


    instr 10000					  ; kill

	    exitnow
  
    endin

</CsInstruments>

<CsScore>
i1 0 10000
e
</CsScore>

</CsoundSynthesizer>
