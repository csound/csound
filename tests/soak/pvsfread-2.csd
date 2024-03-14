<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvsfread-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1 ; play and analyze sound file, but one channel only

idur    filelen   "stereoJungle.wav"        ; check duration of sample  
prints  "\nsound file duration = %f\n", idur
ichn    filenchnls  "stereoJungle.wav"      ; check channels- a stereo sound!
prints  "number of channels = %f\n\n", ichn

asigL, asigR diskin2 "stereoJungle.wav"     ; stereo sound file
fss pvsanal  asigR, 1024, 256, 1024, 0      ; choose which channel- here the Right channel
pvsfwrite fss, "stereoJungle.pvx"           ; write that analyzed channel to pvocex file
ase pvsynth fss                             ; lets hear it
outs ase, ase

endin

instr 2 ; must be called after instr 1 finishes

prints  "\n--**analyzed file is played back**--\n\n"
idur    filelen   "stereoJungle.pvx"        ; find duration of (stereo) analysis file
kpos  line      0,p3,idur                   ; to ensure we process whole file
fsigr pvsfread  kpos,"stereoJungle.pvx", 0  ; read the fsig from this (= Right) channel
aout  pvsynth   fsigr                       ; resynthesise it
outs    aout, aout

endin
</CsInstruments>
<CsScore>
s
i 1 0 7
s
i 2 0 7
i 2 8 4     ; faster
e
</CsScore>
</CsoundSynthesizer>
