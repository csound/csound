; This file is part of BeatHealth_2020_SW.
; 
; Copyright (C) 2018 The National University of Ireland Maynooth, Maynooth University
; 
; The use of the code within this file and all code within files that 
; make up the software that is BeatHealth_2020_SW is permitted for 
; non-commercial purposes only.  The full terms and conditions that 
; apply to the code within this file are detailed within the 
; LICENCE.txt file and at 
; http://www.eeng.nuim.ie/research/BeatHealth_2020_SW/LICENCE.txt
; unless explicitly stated.
; 
; By downloading this file you agree to comply with these terms.
; 
; If you wish to use any of this code for commercial purposes then 
; please email commercialisation@mu.ie

<CsoundSynthesizer>
<CsOptions>
; --- Settings for MotoG 3rd ---
;-odac -dm0 -b960            ;Sync Csound
-odac -dm0 -b480            ;Sync Csound MotoG3 Android 6.0.1, 5.1.1
;-odac -dm0 -b960 -B1920    ;Async Csound
</CsOptions>
<CsInstruments>

;sr = 54000
sr = 48000  ;HTC One M7 , Moto G 3
;sr=44100    ;MotoG
ksmps = 96
; ksmps = 120
0dbfs = 1
nchnls = 2

/*
 Scaling MP3Player instrument
 responds to the following 
 control channels:
 
 - play: play mode (global)
   (1, playing; paused otherwise)
 - fadein: sets start fade in secs (global)
 - fadeout: sets end fade in secs (global)
 - limit: limit instr volume to a given value (global)
 - tempo1.N: track N tempo (per track)
 
 Sets the following channels
 - time1.N: track N file time (ms)
 - timeToEnd1.N: track N file time
    remaining (ms)
 - insttime1.N: track N play time (ms)  
 

e.g.

chnset 1,"play"
chnset 0.9, "tempo1.1"
*/

prealloc 1, 2
chnset 1,"play"
chnset 0.25, "fadeout"
chnset 0,"fadein"
chnset 0.5, "limit"
gip[] init 2

instr 1

ktime init 0
kitim init 0

/*
get instrument name for channels
*/
instname = p1
Stime  sprintf "time%.1f",  instname
Stempo sprintf "tempo%.1f", instname
StimeToEnd sprintf "timeToEnd%.1f", instname
Stimeinsts sprintf "insttime%.1f", instname 

/*
External - Set Tempo
kspeed = playback speed 
(1=normal)
*/
kspeed chnget Stempo
if kspeed <= 0 then
    kspeed = 1
endif

/*
External - Pause track
kplay = playback mode 
(play=1,pause!=1)
*/
kvol init 0
kplay chnget "play"
if kplay == 1 then  
  if kvol < 1 && ktime != 0 then
    /* linear fadein */
    kvol += 0.001  
  else
    kvol = 1
  endif
else /* kplay != 1 */
  if kvol > 0.001 then
    /* expon fadeout */
    kvol *= 0.995
  else 
    a1 = 0
    a2 = 0
    kvol = 0 
    kgoto output
  endif
endif

kpitch = 1
ifadout chnget "fadeout"
ifadin  chnget "fadein"
klimit  chnget "limit"
ifadin = ifadin == 0 ? 1/kr : ifadin
ifadout = ifadout == 0 ? 1/kr : ifadout
amp linsegr 0,ifadin,1,1,1,ifadout, 0

/* al,ar,kt,ilen mp3scal_play ihandle,
                    kspeed,
                    kpitch,
                    kvol[,klock,kintp]
                    
  ihandle should point to a valid mp3 file
  handle provided by mp3scal_load
  
  klock defaults to 1 (phase locking)
  kintp defaults to 1 (use interpolation)

  this op resamples internally, so
  time and pitch adjustments are not
  need to compensate for different file srs
 
  ktime is in seconds and refers to resampled
  file position at the end of the ksmps block
*/
klock = 1
kinterp = 1
iasync = 1
inst = (p1 - int(p1))*10 - 1
a1,a2,ktime,ilen mp3scal_play gip[inst],
                     kspeed,
                     kpitch,
                     klimit,
                     klock,
                     kinterp,
                     iasync
                     
ascal = amp*a(kvol)
output:                     
  outs a1*ascal, a2*ascal

kitim timeinsts
/* times from s to ms */
ilenms = ilen*1000
kitims = kitim*1000
ktims  = ktime*1000
chnset kitims, Stimeinsts
if ktime >= ilen then
  printks "turnoff %d %d %d\n",1,ktims,ilenms, kitims
  chnset ilenms - ktims, Stime
  turnoff2 p1, 4, 1
else
  chnset ktims, Stime
  chnset ilenms - ktims, StimeToEnd
endif

/* position tracing */
;printks "position in ms = %d \n", 0.5, ktims
;i1 chnget Stime
;print i1
endin

chn_k "loaded1",3
chn_k "loaded2",3

instr 2

Sfile strget p4
inst = (p1 - int(p1))*10 - 1

initialoffset init p5
iffts = 1024
idecim = 4
isiz = iffts*8

/* VL 13-9-2016
   resetting time info channel to
   initial offset value
*/
Stime  sprintf "time%.1f",p1-1
chnset initialoffset,Stime

/*
ihandle mp3scal_load Sfile[,iskip,
                         iffts,
                     idecim] 
                     
  loads an mp3 file asynchronously
  iskip defaults to 0s
  ifft to 2048
  idecim to 4
                     
*/
gip[inst] mp3scal_load Sfile,
                     initialoffset/1000,
                     iffts,
                     idecim                    
/* 
kchk mp3scal_check ihandle
   
   checks at perf-time if an 
   mp3 file is loaded

*/                     
kloaded mp3scal_check gip[inst]

if int(inst) == 0 then
 chnset kloaded,"loaded1"                   
else
 chnset kloaded,"loaded2"
endif                    

endin


/* this instrument 
   processes channel messages
   to load and start tracks.
   
   The sequence of calls to
   load a track should be
   (pseudocode):
   
   SetChannel("track1", name);
   SetChannel("offset1", offset);
   SetChannel("load1", 1);
   
   and to play it:
   
   SetChannel("start1", 1);
   
   where name is a string containing
   the channel name and offset is the
   initial skip in ms.
   
   To stop it:
   SetChannel("end1", 1);
   
   Similarly, track2, offset2, start2
   and end2 refer to track 2 control
   channels.
   
*/   
instr 100

Sfile1 chnget "track1"
Sfile2 chnget "track2"
koff1 chnget "offset1"
koff2 chnget "offset2"

kload1 chnget "load1"
if kload1 == 1 then
Sline1 sprintfk {{i2.1 0 -1 "%s" %f}}, Sfile1, koff1
scoreline Sline1,kload1
kload1 = 0
chnset kload1, "load1"

endif

kload2 chnget "load2"
if kload2 == 1 then
Sline2 sprintfk {{i2.2 0 -1 "%s" %f}}, Sfile2, koff2
scoreline Sline2,kload2
kload2 = 0
chnset kload2, "load2"
endif

kstart1 chnget "start1"
if kstart1 == 1 then
scoreline "i1.1 0 -1",kstart1
kstart1 = 0
chnset kstart1, "start1"
endif

kend1 chnget "end1"
if kend1 == 1 then
scoreline "i-1.1 0 1",kend1
kend1 = 0
chnset kend1, "end1"
endif

kstart2 chnget "start2"
if kstart2 == 1 then
scoreline "i1.2 0 -1",kstart2
kstart2 = 0
chnset kstart2, "start2"
endif

kend2 chnget "end2"
if kend2 == 1 then
scoreline "i-1.2 0 0",kend2
kend2 = 0
chnset kend2, "end2"
endif

endin

</CsInstruments>
<CsScore>
;f 0 36000
/* the line below 
   activates track control 
   channel messages
*/
i100 0 36000
/*
or they can be load directly
e.g.
*/

/*
i2.2 1 -1 "/Users/victor/src/csound6/debug/sine.mp3" 500
i1.2 1.1 5
*/

/*
and finished with
i-1.1 0 1 0 0
*/

</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
