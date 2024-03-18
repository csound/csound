<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
;-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
-o vbap8.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

  sr       =          44100
  kr       =           441
  ksmps    =           100
  nchnls   =             4
  vbaplsinit          2, 8,  0, 45, 90, 135, 200, 245, 290, 315 

          instr  1
  asig    oscil       20000, 440, 1
  a1,a2,a3,a4,a5,a6,a7,a8   vbap8   asig, p4, 0, 20 ;p4 = azimuth
	
  ;render twice with alternate outq  statements
  ;  to obtain two 4 channel .wav files:

          outq        a1,a2,a3,a4
  ;       outq        a5,a6,a7,a8
; or use an 8-channel output for realtime output (set nchnls to 8):
;        outo a1,a2,a3,a4,a5,a6,a7,a8
          endin 


</CsInstruments>
<CsScore>
f 1 0 8192 10 1
; Play Instrument #1 for one second.
;          azimuth
i 1 0 1      20
i 1 + .      40
i 1 + .      60
i 1 + .      80
i 1 + .      100
i 1 + .      120
i 1 + .      140
i 1 + .      160
e


</CsScore>
</CsoundSynthesizer>
