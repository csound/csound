<CsoundSynthesizer>
<CsInstruments>
sr     = 44100
ksmps  = 8
nchnls = 2
0dbfs  = 1

giSine        ftgen 1, 0, 8193, 10, 1
giPitchTable  ftgen 2, 0, 16, -7, 45, 16, 45 ; table of MIDI pitches for VCO
giFiltTable   ftgen 3, 0, 16, -7, 69, 16, 69 ; table of MIDI pitches for LPF 
giGateTable   ftgen 4, 0, 32, -7, .001, 32, .001 ; gate env (if all gates on, then it's alternating 1's and .001's)
giAudioTable  ftgen 9, 0, 2, -7, .001, 2, .001 ; temp storage for audio data (so we can read/write at any rate)
giWaveShapeFn ftgen 0,0, 257, 9, .5,1,270 ; used for distortion

chn_k "a", 1, 2, .01, 0.001, 2
chn_k "d", 1, 2, .2,  0.001, 2
chn_k "s", 1, 3, .3,  0.001, 1
chn_k "r", 1, 2, .2,  0.001, 2
chn_k "kVCO2Mult", 1, 2, .5, .125, 4
chn_k "kPlay", 1, 2, 0, 0, 1

#define MAX_DEL_TIME # 6 #

; a-rate MIDI pitch to freq converter
opcode mtofA, a, a
    aIn xin
    aOut = (440.0*exp(log(2.0)*(aIn-69.0)/12.0))
    xout aOut
endop

; k-rate MIDI pitch to freq converter
opcode mtofK, k, k
    kIn xin
    kOut = (440.0*exp(log(2.0)*(kIn-69.0)/12.0))
    xout kOut
endop

; a-rate portamento
opcode porta, a, aki
    setksmps 1
    aIn, khtim, isig xin
    denorm aIn
    kIn downsamp aIn
    kOut portk kIn, khtim, isig
    aOut upsamp kOut
    xout aOut
endop

instr 1
    kSpeed       chnget "speed"              ; Determines speed of table reading.
    kPitchGlide  chnget "pglide"
    kPitchShift  chnget "pshift"
    kFiltGlide   chnget "fglide"
    kFPfollow    chnget "follow"
    kFiltReso    chnget "reso"
    kFiltShift   chnget "fshift"
    kFiltType    chnget "ftype"
    kDelay       chnget "delay"
    kDelayCut    chnget "delCut"
    kDelayFdbk   chnget "delFdbk"
    kDelayAmp    chnget "delAmp"
    kDistMethod  chnget "distMethod"
    kDrive       chnget "drive"
	kWave        chnget "wave"
	kPW          chnget "pw"
	kVCO2		 chnget "o2"  
	kVCO2Mult    chnget "o2mult"
	kVCO2Wave    chnget "o2wav"
	kPlay        chnget "play"
    kSimplePitchFollow chnget "simplefollow"
    kUseFiltTable      chnget "useFiltTable"

           kSpeed = (kSpeed == 0 ? .001 : kSpeed) ; IMPORTANT: Avoid kSpeed == 0.
      kPitchGlide = kPitchGlide * .05             ; Condition glide times.
       kFiltGlide = kFiltGlide * .05

         kOldPlay init 0                          ; Use this to detect changes in playback state.
    
                  if kOldPlay != kPlay && kPlay > 0 then
                  reinit transport                ; Playback is starting, reinit phasor so it starts at phase = zero.
                  endif
        
    transport:                      
           aIndex phasor kSpeed                   ; This phasor drives the sequencer.
    rireturn
            
                  if kPlay == 0 then
           aIndex = 16                            ; Not currently playing, keep index at anything but zero (initial gate value
                  endif
               
         kOldPlay = kPlay
    
           kIndex downsamp aIndex
                  outvalue "idx", kIndex * 16     ; Used to provide some visual feedback regarding sequence position.

           aPitch table aIndex, giPitchTable, 1   ; Get current oscillator pitch.
           aPitch = aPitch + kPitchShift
           aPitch porta aPitch, kPitchGlide, 0    ; Smooth changes in oscillator pitch.
           kPitch downsamp aPitch
             aFrq mtofA aPitch                    ; Convert pitch to frequency.
             aFrq limit aFrq, 40, sr / 2          ; Keep frequency > 30 and less than half of sampling rate.  
            aFrq2 = aFrq * kVCO2Mult
            aFrq2 limit aFrq2, 40, sr / 2

                  if kUseFiltTable > 0 then
       kFiltPitch table kIndex, giFiltTable, 1    ; Get current table filter pitch.
                  else
       kFiltPitch = 0
                  endif

                  if kSimplePitchFollow == 0 then
       kFiltPitch = kFiltPitch + kFiltShift + (kFPfollow * (kPitch-63)) + 63
                  else
       kFiltPitch = kFiltPitch + kFiltShift + (kFPfollow * 12 + kPitch)
                  endif

       kFiltPitch portk kFiltPitch, kFiltGlide    ; Smooth changes in filter pitch.
         kFiltFrq mtofK kFiltPitch
         kFiltFrq limit kFiltFrq, 100, 9000       ; Prevent svfilter from blowing up.

    ; --------------- ADSR section -----------------
        kPrevGate init 0
            aGate table aIndex, giGateTable, 1 ; get the current gate
            kGate downsamp aGate
                  if (kPrevGate < .5) && (kGate > .5) then
                  reinit init_adsr
                  endif
    init_adsr:
        iA        chnget "a"
        iD        chnget "d"
        iS        chnget "s"
        iR        chnget "r"
        iStartAmp tab_i 1, giAudioTable                     ; Obtain last gate env value before starting new env
        aGateEnv  expseg iStartAmp, iA, 1, iD, iS, iR, .001 ; Use last gate env value to start new env (avoids clicks).
    rireturn
        kPrevGate = kGate 
             andx = 1                                       ; tabw needs an a-rate index
         aGateEnv limit aGateEnv, .0001, 1                  ; Prevent aGateEnv from reaching true zero (expseg hates that).
                  tabw aGateEnv, andx, giAudioTable         ; Save current gate value (to be read on next k-pass).
    ; ---------------------------------------------

    ; --------------- VCO section -----------------
                  if kWave < 1 then
             aVCO vco .6, aFrq, 2, kPW, giSine ; square
                  elseif kWave < 2 then 
             aVCO vco .6, aFrq, 1, kPW, giSine ; saw
                  else
             aVCO vco .6, aFrq, 3, kPW, giSine ; tri
                  endif
                  
                  if kVCO2Wave < 1 then
          aVCOsub vco .6, aFrq2, 2, .5, giSine ; square
                  elseif kWave < 2 then 
          aVCOsub vco .6, aFrq2, 1, .5, giSine ; saw
                  else
          aVCOsub vco .6, aFrq2, 3, .5, giSine ; tri
                  endif
                  
             aVCO = aVCO + (aVCOsub * kVCO2)
    ; ---------------------------------------------

    ; --------------- Filter section --------------
    aLP, aHP, aBP svfilter aVCO, kFiltFrq, kFiltReso ; Filter it.
                  if kFiltType < 1 then
               aF = aLP
                  elseif kFiltType < 2 then
               aF = aHP
                  else 
               aF = aBP
                  endif
    ; ---------------------------------------------

    ; --------------- Distortion section ----------
                  if kDistMethod == 0 then
               aD distort aF, kDrive, giWaveShapeFn
                  else
               aD distort1 aF, kDrive, 1, 0, 0, 1            
                  endif
               aD balance aD, aF   
    ; ---------------------------------------------

    ; ---------------------------------------------
               aD = aD * .4 * aGateEnv                ; apply gate env
    ; ---------------------------------------------

    ; ---------------------------------------------                  
             kDel = (1 / (abs(kSpeed) * 16)) * kDelay ; Delay time is multiple of one note (multiple == kDelay).
             kDel limit kDel, .0001, $MAX_DEL_TIME
             aDel upsamp kDel
            aNull delayr $MAX_DEL_TIME                ; Setup delay line with max delay time == $MAX_DEL_TIME seconds.
          aDelOut deltapi aDel                        ; Tap at delay time.
            aFeed butlp aD + aDelOut, kDelayCut       ; Lowpass filter the input + delay tap.
                  delayw aFeed * kDelayFdbk           ; Feed input + delay tap back into delay line.
          aDelOut = aDelOut * kDelayAmp               ; Adjust amplitude of delayed audio.
             aOut = aDelOut + aD
    ; ---------------------------------------------

    ; ---------------------------------------------
                  outch 1, aOut, 2, aOut
    ; ---------------------------------------------                  

endin

</CsInstruments>
<CsScore>
f0 86400
i1 0 86400
e
</CsScore>
</CsoundSynthesizer>

