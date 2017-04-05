<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o abs.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
nchnls = 2

gaReverbSend init 0

; a simple sine wave partial
instr 1
    idur   =       p3
    iamp   =       p4
    ifreq  =       p5
    aenv   linseg  0.0, 0.1*idur, iamp, 0.6*idur, iamp, 0.3*idur, 0.0
    aosc   oscili  aenv, ifreq, 1
           vincr   gaReverbSend, aosc
endin

; global reverb instrument
instr 2
    al, ar reverbsc gaReverbSend, gaReverbSend, 0.85, 12000
           outs     gaReverbSend+al, gaReverbSend+ar
           clear    gaReverbSend
endin

</CsInstruments>
<CsScore>
f1 0 4096 10 1

{ 4 CNT
  { 8 PARTIAL
      ;   start time     duration            amplitude          frequency
      
      i1  [0.5 * $CNT.]  [1 + ($CNT * 0.2)]  [500 + (~ * 200)]  [800 + (200 * $CNT.) + ($PARTIAL. * 20)]
  }
}

i2 0 6
e

</CsScore>
</CsoundSynthesizer>
