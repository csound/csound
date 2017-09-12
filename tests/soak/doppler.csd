<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o doppler.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 128
nchnls = 2
0dbfs = 1

instr 1

iattack       init    0.05
irelease      init    0.05
isustain      init    p3
p3            init    iattack + isustain + irelease
kdamping      linseg  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0	
kmic          init    4
              ; Position envelope, with a changing rate of change of position.
;             transeg a   dur   ty  b      dur    ty  c    dur    ty d
kposition     transeg 4, p3*.4, 0, 120,   p3*.3, -3, 50,   p3*.3, 2, 4
ismoothinghz  init    6
ispeedofsound init    340.29
asignal       vco2    0.5, 110
aoutput       doppler asignal, kposition, kmic, ispeedofsound, ismoothinghz
              outs    aoutput*kdamping, aoutput * kdamping
endin

</CsInstruments>
<CsScore>

i1	0.0	20	
e1
</CsScore>
</CsoundSynthesizer>