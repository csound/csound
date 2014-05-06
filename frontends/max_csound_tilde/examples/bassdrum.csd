<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 4
nchnls  = 1
0dbfs   = 1
massign 0, 0 ; Disable default MIDI assignments.
massign 1, 1 ; Assign MIDI channel 1 to instr 1.

giSine       ftgen 1, 0, 16384, 10, 1 ; Generate a sine wave table.
giTri        ftgen 2, 0, 16384, 7, 0, 4096, 1, 4096, 0, 4096, -1, 4096, 0
giSquare9    ftgen 3, 0, 16385, 9, 1, 1, 0, 3, 1/3, 0, 5, 1/5, 0, 7, 1/7, 0, 9, 1/9, 0, 11, 1/11, 0, 13, 1/13, 0, 15, 1/15, 0, 17, 1/17, 0
giSaw9       ftgen 4, 0, 16385, 9, 1, 1, 0, 2, 1/2, 0, 3, 1/3, 0, 4, 1/4, 0, 5, 1/5, 0, 6, 1/6, 0, 7, 1/7, 0, 8, 1/8, 0, 9, 1/9, 0, 10, 1/10, 0, 11, 1/11, 0, 12, 1/12, 0, 13, 1/13, 0, 14, 1/14, 0

giBus        ftgen 100, 0, 4, -17, .001, 4, .001
giAmpIndex   init 0 ; an index into giBus table

#define FreqMin # 50 #
#define FreqMax # 5000 #
#define Dur # 7200 #

chn_k "frqDec", 1  
chn_k "frqRel", 1
chn_k "frqSus", 1  
chn_k "frqBase", 1
chn_k "frqRange", 1
chn_k "ampAtk", 1  
chn_k "ampDec", 1  
chn_k "ampRel", 1
chn_k "ampSus", 1
chn_k "filtFrqMult", 1  
chn_k "filtRes", 1
chn_k "filt", 1
chn_k "wave", 1 ; IMPORTANT: If "wave" is not declared here, the oscilikts line in instr #4 may fail.
chn_k "drive", 1
chn_k "clipType", 1

gaOscFrq init 0
gaSync init 1

opcode ptofK, k, kii
    kIn, iMin, iMax xin
    iMaxOverMin = iMax / iMin
    kIn limit kIn, 0, 1
    kOut = iMin * (iMaxOverMin ^ kIn)
    xout kOut
endop

opcode ptofA, a, aii
    setksmps 1
    aIn, iMin, iMax xin
    iMaxOverMin = iMax / iMin
    kIn downsamp aIn
    kIn limit kIn, 0, 1
    kOut = iMin * (iMaxOverMin ^ kIn)
    aOut upsamp kOut
    xout aOut
endop

; MIDI activated
instr 1
    event_i "i", 3, 0, $Dur ; Activate instr 3 with a REALLY long duration.
endin

; This instrument is used mainly to init a-rate values
; every k-cycle. It's always-on.
instr 2
    gaOscAmp tab giAmpIndex, giBus ; Set gaOscAmp to last known oscillator amplitude.
endin

; Envelope instrument for instr #4.  Activated by instr #1.
instr 3
    iFrqDec    chnget "frqDec"   ; in secs
    iFrqRel    chnget "frqRel"   ; in secs
    iFrqSus    chnget "frqSus"   ; [0,1]
    iFrqBase   chnget "frqBase"  ; [0,1]
    iFrqRange  chnget "frqRange" ; [0,1]
    iAmpAtk    chnget "ampAtk"   ; in secs
    iAmpDec    chnget "ampDec"   ; in secs
    iAmpRel    chnget "ampRel"   ; in secs
    iAmpSus    chnget "ampSus"   ; [0,1]
    iAmpStart  table giAmpIndex, giBus
    iAmpAtk    = (iAmpAtk < 0.0001 ? 0.0001 : iAmpAtk)
    iAmpStart  = (iAmpStart < 0.0001 ? 0.0001 : iAmpStart)

    gaOscPitch expseg 1, 0.0001, 1, iFrqDec, iFrqSus, iFrqRel, 0.0001
    gaOscPitch = (gaOscPitch * iFrqRange) + iFrqBase    
    gaOscPitch limit gaOscPitch, 0, 1
    gaOscFrq   ptofA gaOscPitch, $FreqMin, $FreqMax
    gaOscAmp   expseg iAmpStart, iAmpAtk, 1, iAmpDec, iAmpSus, iAmpRel, 0.0001
    gaSync     init 1
endin

; Always-on signal generator instrument.
instr 4
    kinstr_num init 3
    kactive active kinstr_num
    if kactive > 1 then
        turnoff2 3, 1, 0  ; Turn off older instance without release.
    endif

    ; Generate source wave.
    kFn    chnget "wave"
    kFn    limit kFn, 1, 4
    aOsc   oscilikts gaOscAmp, gaOscFrq, kFn, gaSync, 0
    gaSync = 0 ; Reset the sync flag to zero.
           denorm aOsc
    
    kFilt chnget "filt"
    if kFilt > 0 then
        kFiltPitchMult chnget "filtFrqMult"
        kFiltRes       chnget "filtRes"
        kOscPitch      downsamp gaOscPitch
        kFiltPitch     = (kOscPitch + .05) * kFiltPitchMult
        kFiltFrq       ptofK kFiltPitch, $FreqMin, $FreqMax
        aOsc           moogladder aOsc, kFiltFrq, kFiltRes    
    endif

    kDrive    chnget "drive"
    kClipType chnget "clipType"
    kDrive    portk kDrive, 0.05

    if kClipType == 0 then
        aClip clip aOsc * kDrive, 0, .5, .1
    elseif kClipType == 1 then
        aClip clip aOsc * kDrive, 1, .5 
    else 
        aClip clip aOsc * kDrive, 2, .5
    endif

    kAmp chnget "amp"
    outch 1, aClip * kAmp
    
    aNdx init giAmpIndex
    tabw gaOscAmp, aNdx, giBus ; keep track of current Oscillator amplitude in giBus table
endin

</CsInstruments>
<CsScore>
f0 3600
i2 0 3600
i4 0 3600
e
</CsScore>
</CsoundSynthesizer>


