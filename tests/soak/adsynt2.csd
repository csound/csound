<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o adsynt2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1
; Generate a sinewave table.
giwave ftgen 1, 0, 1024, 10, 1
; Generate two empty tables for adsynt2.
gifrqs ftgen 2, 0, 32, 7, 0, 32, 0
; A table for freqency and amp parameters.
giamps ftgen 3, 0, 32, 7, 0, 32, 0
  
; Generates parameters at init time
instr 1
  ; Generate 10 voices.
  icnt = 10 
  ; Init loop index.
  index = 0 

; Loop only executed at init time.
loop: 
  ; Define non-harmonic partials.
  ifreq pow index + 1, 1.5 
  ; Define amplitudes.
  iamp = 1 / (index+1) 
  ; Write to tables.
  tableiw ifreq, index, gifrqs 
  ; Used by adsynt2.
  tableiw iamp, index, giamps 
  
  index = index + 1
  ; Do loop/
  if (index < icnt) igoto loop 
  
  asig adsynt2 0.4, 150, giwave, gifrqs, giamps, icnt
  outs asig, asig
endin

; Generates parameters every k-cycle.
instr 2 
  ; Generate 10 voices.
  icnt = 10 
  ; Reset loop index.
  kindex = 0

; Loop executed every k-cycle.
loop:
  ; Generate lfo for frequencies.
  kspeed  pow kindex + 1, 1.6
  ; Individual phase for each voice.
  kphas phasorbnk kspeed * 0.7, kindex, icnt
  klfo table kphas, giwave, 1
  ; Arbitrary parameter twiddling...
  kdepth pow 1.4, kindex
  kfreq pow kindex + 1, 1.5
  kfreq = kfreq + klfo*0.006*kdepth

  ; Write freqs to table for adsynt2.
  tablew kfreq, kindex, gifrqs 
  
  ; Generate lfo for amplitudes.
  kspeed  pow kindex + 1, 0.8
  ; Individual phase for each voice.
  kphas phasorbnk kspeed*0.13, kindex, icnt, 2
  klfo table kphas, giwave, 1
  ; Arbitrary parameter twiddling...
  kamp pow 1 / (kindex + 1), 0.4
  kamp = kamp * (0.3+0.35*(klfo+1))

  ; Write amps to table for adsynt2.
  tablew kamp, kindex, giamps
  
  kindex = kindex + 1
  ; Do loop.
  if (kindex < icnt) kgoto loop

  asig adsynt2 0.25, 150, giwave, gifrqs, giamps, icnt
  outs asig, asig
endin


</CsInstruments>
<CsScore>

; Play Instrument #1 for 2.5 seconds.
i 1 0 2.5
; Play Instrument #2 for 2.5 seconds.
i 2 3 2.5
e


</CsScore>
</CsoundSynthesizer>
