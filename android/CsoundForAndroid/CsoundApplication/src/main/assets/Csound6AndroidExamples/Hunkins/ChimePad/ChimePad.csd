; CHIMEPAD (2012) for realtime Android Csound - by Arthur B. Hunkins
;   A custom version of CHIMEPLAY for chime and other natural sound samples
; Requires Android OS 2.3 or higher and CsoundApp-5.18.apk (or higher) installed
    Downloadable from http://sourceforge.net/projects/csound/files/csound5/Android/
    Run CSD Player and Browse to this file; select one of the two sets before starting. 
; 0-1 background loop, 1-5 single-shot samples per sound set
;   Only WAV files are permitted.
; Each default sound set includes 5 chimes and an ambient loop. 
;   Background loop must be named 0.wav (set 1) or 10.wav (set 2),
;   and samples labeled 1.wav and up (set 1), or 11.wav and up (set 2).
;   All soundfiles must be in the same folder as this file (suggestion: Music folder).
; Users are encouraged to create their own sound sets from natural sources.
; For more info, including a sample performance outline, see: ChimePadReadMe.txt

<CsoundSynthesizer>
<CsOptions>

-odac -dm3 --expression-opt --sample-accurate

</CsOptions>
<CsInstruments>


sr      = 8000
ksmps   = 100
nchnls  = 2

gkamp   init   1
gkpan   init   .5

	instr 1

kflag   init   0
kflag1  init   0
kflag2  init   0
kflag3  init   0
kflag4  init   0
kflag5  init   0
ktime1  init   0
ktime2  init   0
ktime3  init   0
ktime4  init   0
ktime5  init   0
ktime   times
iset    chnget "slider1"
        if (iset > .75) || (iset < .25) then
gidecay =         2
        else
gidecay =         .025        
        endif
        if iset > .5 then
Sfile0  =         "10.wav"        
ins0    =         12
Sfile1  =         "11.wav"        
ins1    =         13
Sfile2  =         "12.wav"        
ins2    =         14
Sfile3  =         "13.wav"        
ins3    =         15
Sfile4  =         "14.wav"        
ins4    =         16
Sfile5  =         "15.wav"        
ins5    =         17
        else
Sfile0  =         "0.wav"        
ins0    =         2
Sfile1  =         "1.wav"        
ins1    =         3
Sfile2  =         "2.wav"        
ins2    =         4
Sfile3  =         "3.wav"        
ins3    =         5
Sfile4  =         "4.wav"        
ins4    =         6
Sfile5  =         "5.wav"        
ins5    =         7
        endif
i0      filevalid Sfile0
i1      filevalid Sfile1
        if i1 == 1 then
i1a     filelen   Sfile1
        endif        
i2      filevalid Sfile2
        if i2 == 1 then
i2a     filelen   Sfile2
        endif        
i3      filevalid Sfile3
        if i3 == 1 then
i3a     filelen   Sfile3
        endif        
i4      filevalid Sfile4
        if i4 == 1 then
i4a     filelen   Sfile4
        endif        
i5      filevalid Sfile5
        if i5 == 1 then
i5a     filelen   Sfile5
        endif        
        if (i0 == 0) || (kflag == 1) goto skip
        event  "i", ins0, 0, -1
kflag   =       1
skip:
kamp    chnget  "trackpad.y"
gkamp   =       (kamp > 0? kamp: gkamp)
kpan    chnget  "trackpad.x"
gkpan   =       (kpan > 0? kpan: gkpan)
kfreq   chnget  "slider2"
        if kfreq < .05 then
kflag1  =       0 
kflag2  =       0 
kflag3  =       0 
kflag4  =       0 
kflag5  =       0 
        endif
        if i1 > 0 then
k1      chnget  "butt1"
k1a     trigger k1, .5, 0
        if k1a > 0 then
        event   "i", ins1, 0, i1a
        kgoto end
        endif
        if kfreq >= .05 then
        if kflag1 == 0 then
krand1  random  2, 2 + (kfreq * 18)
ktime1  =       ktime + krand1
kflag1  =       1
        kgoto end
        endif
        if ktime >= ktime1 then
        event   "i", ins1, 0, i1a
kflag1  =       0
        kgoto end
        endif
        endif
        endif
        if i2 > 0 then
k2      chnget  "butt2"
k2a     trigger k2, .5, 0
        if k2a > 0 then
        event   "i", ins2, 0, i2a
        kgoto end
        endif
        if kfreq >= .05 then
        if kflag2 == 0 then
krand2  random  2, 2 + (kfreq * 18)
ktime2  =       ktime + krand2
kflag2  =       1
        kgoto end
        endif
        if ktime >= ktime2 then
        event   "i", ins2, 0, i2a
kflag2  =       0
        kgoto end
        endif
        endif
        endif
        if i3 > 0 then
k3      chnget  "butt3"
k3a     trigger k3, .5, 0
        if k3a > 0 then
        event   "i", ins3, 0, i3a
        kgoto end
        endif
        if kfreq >= .05 then
        if kflag3 == 0 then
krand3  random  2, 2 + (kfreq * 18)
ktime3  =       ktime + krand3
kflag3  =       1
        kgoto end
        endif
        if ktime >= ktime3 then
        event   "i", ins3, 0, i3a
kflag3  =       0
        kgoto end
        endif
        endif
        endif
        if i4 > 0 then
k4      chnget  "butt4"
k4a     trigger k4, .5, 0
        if k4a > 0 then
        event   "i", ins4, 0, i4a
        kgoto end
        endif
        if kfreq >= .05 then
        if kflag4 == 0 then
krand4  random  2, 2 + (kfreq * 18)
ktime4  =       ktime + krand4
kflag4  =       1
        kgoto end
        endif
        if ktime >= ktime4 then
        event   "i", ins4, 0, i4a
kflag4  =       0
        kgoto end
        endif
        endif
        endif
        if i5 > 0 then
k5      chnget  "butt5"
k5a     trigger k5, .5, 0
        if k5a > 0 then
        event   "i", ins5, 0, i5a
        kgoto end
        endif
        if kfreq >= .05 then
        if kflag5 == 0 then
krand5  random  2, 2 + (kfreq * 18)
ktime5  =       ktime + krand5
kflag5  =       1
        kgoto end
        endif
        if ktime >= ktime5 then
        event   "i", ins5, 0, i5a
kflag5  =       0
        kgoto end
        endif
        endif
        endif
        
end:    endin

        instr 2, 12

kamp    init   0
        if p1 == 2 then
Sfile   =       "0.wav"
        else
Sfile   =       "10.wav"
        endif                
ichans  filenchnls Sfile
kamp	chnget "slider5"
skip4:
        if ichans == 2 goto skip5
aout    diskin2 Sfile, 1, 0, 1
        outs    aout * kamp * 4, aout * kamp * 4
        goto end
skip5:
a1, a2  diskin2 Sfile, 1, 0, 1
        outs    a1 * kamp * 4, a2 * kamp * 4

end:    endin

        instr 3, 4, 5, 6, 7, 13, 14, 15, 16, 17

iamp    =       i(gkamp)
iramp   chnget  "slider4"
iramp2  =       1 - iramp
iramp2  random  iramp2, 1
iamp    =       (iramp < .05? iamp: iramp2)
ipan    =       i(gkpan)
irpan   chnget  "slider3"
irpan2  =       irpan * .5
irpan2  random  -irpan2, irpan2
ipan    =       (irpan < .05? ipan: .5 + irpan2)
        if p1 == 3 then
Sfile   =       "1.wav"
        elseif p1 == 4 then
Sfile   =       "2.wav"
        elseif p1 == 5 then
Sfile   =       "3.wav"
        elseif p1 == 6 then
Sfile   =       "4.wav"
        elseif p1 == 7 then
Sfile   =       "5.wav"
        elseif p1 == 13 then
Sfile   =       "11.wav"
        elseif p1 == 14 then
Sfile   =       "12.wav"
        elseif p1 == 15 then
Sfile   =       "13.wav"
        elseif p1 == 16 then
Sfile   =       "14.wav"
        else
Sfile   =       "15.wav"
        endif                
ichans  filenchnls Sfile
kamp    linseg  0, .025, iamp, p3 - .025 - gidecay, iamp, gidecay, 0
        if ichans == 2 goto skip
aout    diskin2 Sfile, 1
a1,a2,a3,a4 pan aout, ipan, 1, 1, 1
        outs    a1 * kamp, a2 * kamp
        goto end
skip:
aout,aout2 diskin2 Sfile, 1
a1,a2,a3,a4 pan aout, ipan, 1, 1, 1
a5,a6,a7,a8 pan aout2, ipan, 1, 1, 1
        outs    (a1 + a5) * kamp, (a2 + a6) * kamp
                
end:    endin
               
</CsInstruments>

<CsScore>

f1 0 8193 9 .25 1 0
i1 0 36000

e

</CsScore>
</CsoundSynthesizer>
