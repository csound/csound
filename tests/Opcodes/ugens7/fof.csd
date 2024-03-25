<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o fof.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32  
nchnls = 2

instr 1
  ; Combine five formants together to create 
  ; a transformation from an alto-"a" sound
  ; to an alto-"i" sound.
  ; Values common to all of the formants.
  kfund init 261.659
  koct init 0
  kris init 0.003
  kdur init 0.02
  kdec init 0.007
  iolaps = 100
  ifna = 1
  ifnb = 2
  itotdur = p3

  ; First formant.
  k1amp = ampdb(0)
  k1form line 800, p3, 350
  k1band line 80, p3, 50

  ; Second formant.
  k2amp line ampdb(-4), p3, ampdb(-20)
  k2form line 1150, p3, 1700
  k2band line 90, p3, 100

  ; Third formant.
  k3amp line ampdb(-20), p3, ampdb(-30)
  k3form line 2800, p3, 2700
  k3band init 120

  ; Fourth formant.
  k4amp init ampdb(-36)
  k4form line 3500, p3, 3700
  k4band line 130, p3, 150

  ; Fifth formant.
  k5amp init ampdb(-60)
  k5form init 4950
  k5band line 140, p3, 200

  a1 fof k1amp, kfund, k1form, koct, k1band, kris, \
         kdur, kdec, iolaps, ifna, ifnb, itotdur
  a2 fof k2amp, kfund, k2form, koct, k2band, kris, \
         kdur, kdec, iolaps, ifna, ifnb, itotdur
  a3 fof k3amp, kfund, k3form, koct, k3band, kris, \
         kdur, kdec, iolaps, ifna, ifnb, itotdur
  a4 fof k4amp, kfund, k4form, koct, k4band, kris, \
         kdur, kdec, iolaps, ifna, ifnb, itotdur
  a5 fof k5amp, kfund, k5form, koct, k5band, kris, \
         kdur, kdec, iolaps, ifna, ifnb, itotdur

  ; Combine all of the formants together
asig sum (a1+a2+a3+a4+a5) * 13000
     outs asig, asig

endin
</CsInstruments>
<CsScore>
; sine wave
f 1 0 4096 10 1
; sigmoid wave
f 2 0 1024 19 0.5 0.5 270 0.5

i 1 0 1
i 1 2 5	; same but slower
e
</CsScore>
</CsoundSynthesizer>
