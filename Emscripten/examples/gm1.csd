<CsoundSynthesizer>
<CsOptions>
-odac -d -M 0
</CsOptions>
<CsInstruments>
nchnls=2
ksmps = 64
           

ichn = 1
lp1: massign   ichn, 0
loop_le   ichn, 1, 16, lp1
pgmassign 0, 0
gisf   sfload "sf_GMbank.sf2"
sfpassign  0, gisf

/* this instrument parses MIDI input
   to trigger the GM soundfont synthesis
   instrument (instr 10
*/
instr 1
idkit = 317 /* drum-kit preset was 317*/
tableiw idkit, 9, 1
irel = 0.5 /* release envelope */

ipg = 1
ivol = 2
ipan = 3

nxt:
  kst, kch, kd1, kd2 midiin

  if (kst != 0) then
    kch = kch - 1
    if (kst == 144 && kd2 != 0) then ; note on
        kpg table kch, ipg 
        /* instrument identifier is 10.[chn][note] */
        kinst = 10 + kd1/100000 + kch/100  
        if kch == 9 then
         /* exclusive identifiers */
         if kpg == idkit+7 then
           krel = 2    /* add extra release time for orch perc*/
         else
           krel = 0.5
         endif
         if (kd1 == 29 || kd1 == 30) then ; EXC7
          kinst = 10.97
         elseif (kd1 == 42 || kd1 == 44 || kd1 == 46 || kd1 == 49) then ; EXC1
           kinst = 10.91
         elseif (kd1 == 71 || kd1 == 72) then ; EXC2         
           kinst = 10.92
         elseif (kd1 == 73 || kd1 == 74) then ; EXC3         
           kinst = 10.93
         elseif (kd1 == 78 || kd1 == 79) then ; EXC4         
           kinst = 10.94
         elseif (kd1 == 80 || kd1 == 81) then ; EXC5         
           kinst = 10.95
         elseif (kd1 == 86 || kd1 == 87) then ; EXC6         
           kinst = 10.96
         endif
        else
         krel = 0.5
        endif
        event "i", kinst, 0, -1, kd1, kd2, kpg, kch,krel  
     
    elseif (kst == 128 || (kst == 144 && kd2 == 0)) then ; note off
        kpg table kch, ipg
        kinst = 10 +  kd1/100000 + kch/100
        if kch == 9 then
         if (kd1 == 29 || kd1 == 30) then ; EXC7
          kinst = 10.97
         elseif (kd1 == 42 || kd1 == 44 || kd1 == 46 || kd1 == 49) then ; EXC1
           kinst = 10.91
         elseif (kd1 == 71 || kd1 == 72) then ; EXC2         
           kinst = 10.92
         elseif (kd1 == 73 || kd1 == 74) then ; EXC3         
           kinst = 10.93
         elseif (kd1 == 78 || kd1 == 79) then ; EXC4         
           kinst = 10.94
         elseif (kd1 == 80 || kd1 == 81) then ; EXC5         
           kinst = 10.95
         elseif (kd1 == 86 || kd1 == 87) then ; EXC6         
           kinst = 10.96
         endif
        else
         kpg = 0
        endif
        event "i", -kinst, 0, 1 
     
    elseif (kst == 192) then /* program change msgs */
       if kch == 9 then
         kpg = idkit
         if kd1 == 8 then
         kpg = idkit+1
         elseif kd1 == 16 then
         kpg = idkit+2
         elseif kd1 == 24 then
         kpg = idkit+3
         elseif kd1 == 25 then
         kpg = idkit+4
         elseif kd1 == 32 then
         kpg = idkit+5
         elseif kd1 == 40 then
         kpg = idkit+6
         elseif kd1 == 48 then
         kpg = idkit+7
         endif
       else
       kpg = kd1 
       endif
       tablew  kpg, kch, ipg
    elseif (kst == 176 && kd1 == 11) then /* volume msgs */
       tablew kd2, kch, ivol
    elseif (kst == 176 && kd1 == 7) then /* pan msgs    */
       tablew kd2, kch, ipan
    endif
     kgoto nxt
  endif

endin

/* this is the GM soundfont synthesizer instrument */
instr 10
kenv linenr 10,0.001,p8,0.001
iamp table p5, 5
a1, a2 sfplay p5, p4, iamp,1, p6, 0, 0, 2
kv table p7, 2
kvol tablei kv, 5 
kpan  table p7, 3
kpan = (kpan - 64)/128
       outs 0.2*a1*kvol*(0.5-kpan/2)*kenv, 0.2*a2*kvol*(0.5+kpan/2)*kenv 
endin

instr 11

endin

</CsInstruments>
<CsScore>
/* program preset (memory) table */
f1 0 16 -2 0 0 0 0 0 0 0 0 226 0 0 0 0 0 0 0
/* velocity (memory) table */ 
f2 0 16 -2 127 127 127 127 127 127 127 127 127 127 127 127 127 127 127 127 
/* pan (memory) table */
f3 0 16 -2 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 64 
f5 0 128 5 0.1 128 1   /* velocity mapping: less nuanced */
f6 0 128 5 0.01  128 1 /* velocity mapping: more nuanced */
i 1 0 360000
e
</CsScore>
</CsoundSynthesizer> 
