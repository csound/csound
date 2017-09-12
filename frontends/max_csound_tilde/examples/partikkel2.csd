<CsoundSynthesizer>
/*
    This CSD is based on the partikkel example written by Joachim Heintz and Oeyvind Brandtsegg.
    Some of it was written by others, but I forgot who.  Sorry :(
*/
<CsInstruments>
sr      = 44100
ksmps   = 16
nchnls  = 2   

0dbfs = 1

; IMPORTANT: giFile must be table # 1 for the partikkel patch to work.
giFile      ftgen   1, 0, 1048577, 7, 0, 1048576, 0 ; A blank table to be filled with audio data using "loadsamp" message.
giCosine    ftgen   200, 0, 8193, 9, 1, 1, 90       ; cosine
giDisttab   ftgen   201, 0, 32768, 7, 0, 32768, 1   ; for kdistribution
giPan       ftgen   202, 0, 32768, -21, 1           ; for panning (random values between 0 and 1)

#define WINDOW_TABLE_BASE # 100 #
giWinHamming   ftgen 100, 0, 4096, 20, 1 ; Hamming
giWinHanning   ftgen 101, 0, 4096, 20, 2 ; Hanning
giWinBartlett  ftgen 102, 0, 4096, 20, 3 ; Bartlett
giWinBlackman  ftgen 103, 0, 4096, 20, 4 ; Blackman
giWinBlackHar  ftgen 104, 0, 4096, 20, 5 ; Blackman - Harris
giWinGaussian  ftgen 105, 0, 4096, 20, 6 ; Gaussian
giWinSync      ftgen 106, 0, 4096, 20, 9 ; Sync
giWinPerc      ftgen 107, 0, 512, 7, 0, 12, 1, 500, 0       ; PERCUSSIVE - STRAIGHT SEGMENTS
giWinPercExp   ftgen 108, 0, 512, 5, .001, 12, 1, 500, .001 ; PERCUSSIVE - EXPONENTIAL SEGMENTS
giWinGate      ftgen 109, 0, 512, 7, 0, 6, 1, 500, 1, 6, 0  ; GATE - WITH ANTI-CLICK RAMP UP AND RAMP DOWN SEGMENTS
giWinRevPerc   ftgen 110, 0, 512, 7, 0, 500, 1, 12, 0      ; REVERSE PERCUSSIVE - STRAIGHT SEGMENTS
giWinRevPerc2  ftgen 111, 0, 512, 5, .001, 500, 1, 12, .001 ; REVERSE PERCUSSIVE - EXPONENTIAL SEGMENTS

instr 1
    ipan  init 0            ; panning narrow 0 to wide 1
    kdist chnget "k_g_dist" ; grain distribution (0=periodic, 1=scattered)
    iWin  chnget "i_win"    ; The window index to use for grains.  (0 is minimum)
    iWin  = iWin + $WINDOW_TABLE_BASE

    kGrainAmp  chnget "k_g_amp"
    kGrainRate chnget "k_g_rate"  ; grains per second
    kGrainSize chnget "k_g_size"  ; grain size in ms
    iTrans     chnget "k_g_trans" ; transposition 
    kTrans     chnget "k_g_trans" ; transposition
    kposrand   chnget "k_posRand" ; time position randomness (offset) of the pointer in ms
    iPos       chnget "k_pos"     ; normalized position
    kPos       chnget "k_pos"     ; normalized position
    kPosGlide  chnget "k_posGlide"; If > 0, then apply glide to changes in position, == glide time
    kPortTime  chnget "k_port"  
    kPortTime  = kPortTime * .5

    ; Get length of source wave file, needed for both transposition and time pointer.
    ; NOTE: Since we're using the "loadsamp" message to load audio data into table giFile,
    ; associated table data like nsmps will be incorrect.
    iFileLength     tableng giFile
    iFileDur        = iFileLength / sr
    iFileDurInvert  = 1 / iFileDur
    
    ; Sync Input
    async           = 0     
    
    ; Grain Envelope
    kenv2amt        = 1          ; use only secondary envelope
    ienv2tab        = iWin       ; grain (secondary) envelope
    ienv_attack     = -1         ; default attack envelope (flat)
    ienv_decay      = -1         ; default decay envelope (flat)
    ksustain_amount = 0.5        ; no meaning in this case (use only secondary envelope, ienv2tab)
    ka_d_ratio      = 0.5        ; no meaning in this case (use only secondary envelope, ienv2tab)
    
    ; Amplitude
    kamp            = kGrainAmp  ; grain amplitude
    igainmasks      = -1         ; (default) no gain masking
    aExtEnv         = 1
    
    ; Transposition
    iorig           = iFileDurInvert ; original pitch
    kTrans          portk kTrans, kPortTime, iTrans
    kwavfreq        = iorig * kTrans
    
    ; Other Pitch Related Stuff
    ksweepshape     = 0          ; no frequency sweep
    iwavfreqstarttab= -1         ; default frequency sweep start
    iwavfreqendtab  = -1         ; default frequency sweep end
    awavfm          = 0          ; no FM input
    ifmamptab       = -1         ; default FM scaling (=1)
    kfmenv          = -1         ; default FM envelope (flat)
    
    ; Trainlet Related
    icosine         = giCosine   ; cosine ftable
    kTrainCps       = kGrainRate ; set trainlet cps equal to grain rate for single-cycle trainlet in each grain
    knumpartials    = 1          ; number of partials in trainlet
    kchroma         = 1          ; balance of partials in trainlet
    
    ; Panning Using Channel Masks
    imid            = .5         ; center
    ileftmost       = imid - ipan * .5
    irightmost      = imid + ipan * .5
    giPanthis       ftgen    0, 0,  32768, -24, giPan, ileftmost, irightmost    ; rescales giPan according to ipan
                    tableiw  0, 0,  giPanthis               ; change index 0 ...
                    tableiw  32766, 1, giPanthis            ; ... and 1 for ichannelmasks
    ichannelmasks   = giPanthis                             ; ftable for panning
    
    ; Random Gain Masking
    krandommask     = 0 
    
    ; Source Waveforms
    kwaveform1      = giFile     ; source waveform
    kwaveform2      = giFile     ; all 4 sources are the same
    kwaveform3      = giFile
    kwaveform4      = giFile
    iwaveamptab     = -1         ; (default) equal mix of source waveforms and no amplitude for trainlets
    
    ; Time Pointer
	kPos = (kPos * sr) / ftlen(giFile)
    kPos lineto kPos, kPosGlide
    afilposphas upsamp kPos   
    afilposphas wrap afilposphas, 0, 1 

    ; Generate Random Deviation of the Time Pointer
    kposrandsec     = kposrand * .001   ; ms -> sec
    kposrand        = kposrandsec * iFileDurInvert ; phase values (0-1)
    krndpos         linrand  kposrand   ; random offset in phase values
    
    ; Add Random Deviation to the Time Pointer
    asamplepos1     = afilposphas + krndpos; resulting phase values (0-1)
    asamplepos2     = asamplepos1
    asamplepos3     = asamplepos1   
    asamplepos4     = asamplepos1   
    
    ; Original Key for Each Source Waveform
    kwavekey1       = 1
    kwavekey2       = kwavekey1 
    kwavekey3       = kwavekey1
    kwavekey4       = kwavekey1
    
    ; Maximum Number of Grains per K Period
    imax_grains     = 100       

    aL, aR  partikkel kGrainRate, kdist, giDisttab, async, kenv2amt, ienv2tab, \
        ienv_attack, ienv_decay, ksustain_amount, ka_d_ratio, kGrainSize, kamp, igainmasks, \
        kwavfreq, ksweepshape, iwavfreqstarttab, iwavfreqendtab, awavfm, \
        ifmamptab, kfmenv, icosine, kTrainCps, knumpartials, \
        kchroma, ichannelmasks, krandommask, kwaveform1, kwaveform2, kwaveform3, kwaveform4, \
        iwaveamptab, asamplepos1, asamplepos2, asamplepos3, asamplepos4, \
        kwavekey1, kwavekey2, kwavekey3, kwavekey4, imax_grains

        outs aL * aExtEnv, aR * aExtEnv
endin

; This instr turns off instr 1.
; The turnoff2 command is only executed once.
instr 2
	turnoff2 1, 0, 0
	turnoff
endin

</CsInstruments>
<CsScore>
f0 86400
;i1 0 86400 ; Removed by D. Pyon.
e
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
