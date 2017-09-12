<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o i_statement.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 10
        
  icps     init      cpspch(p4)                  ; Get target pitch from score event
  iportime init      abs(p3)/7                   ; Portamento time dep on note length
  iamp0    init      p5                          ; Set default amps
  iamp1    init      p5
  iamp2    init      p5
      
  itie     tival                                 ; Check if this note is tied,
  if itie  ==  1     igoto nofadein              ; if not fade in
  iamp0    init      0

nofadein:
  if p3    < 0       igoto nofadeout             ; Check if this note is held, if not fade out
  iamp2    init      0    

nofadeout:
  ; Now do amp from the set values:
  kamp     linseg    iamp0, .03, iamp1, abs(p3)-.03, iamp2
        
  ; Skip rest of initialization on tied note:
           tigoto    tieskip

  kcps     init      icps                        ; Init pitch for untied note
  kcps     port      icps, iportime, icps        ; Drift towards target pitch

  kpw      oscil     .4, rnd(1), 1, rnd(.7)      ; A simple triangle-saw oscil
  ar       vco       kamp, kcps, 3, kpw+.5, 1, 1/icps
        
  ; (Used in testing - one may set ipch to cpspch(p4+2)
  ;       and view output spectrum)
  ;       ar oscil kamp, kcps, 1

          outs        ar, ar

tieskip:                                        ; Skip some initialization on tied note

endin
</CsInstruments>
<CsScore>
f1   0 8192 10 1            ; Sine

i10.1    0    -1    7.00    .15
i10.2    0    -1    7.04
i10.3    0    -1    7.07
i10.1    1    -1    8.00  
i10.2    1    -1    8.04
i10.3    1    -1    8.07
i10.1    2     1    7.11  
i10.2    2     1    8.04  
i10.3    2     1    8.07
  e
</CsScore>
</CsoundSynthesizer>

