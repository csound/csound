<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o vincr-complex.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gaReverbSend init 0

instr 1

iamp   = p4
ifreq  = p5
aenv   linseg  0.0, 0.1*p3, iamp, 0.6*p3, iamp, 0.3*p3, 0.0
aosc   poscil aenv, ifreq, 1
       vincr   gaReverbSend, aosc
endin


instr 2	; global reverb instrument

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
      i1  [0.5 * $CNT.]  [1 + ($CNT * 0.2)]  [.04 + (~ * .02)]  [800 + (200 * $CNT.) + ($PARTIAL. * 20)]
  }
}

i2 0 6
e
</CsScore>
</CsoundSynthesizer>

