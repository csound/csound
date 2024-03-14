<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o resonz.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

/* Written by Sean Costello */
 ; Orchestra file for resonant filter sweep of a sawtooth-like waveform. 
  ; The outputs of reson, resonr, and resonz are scaled by coefficients
  ; specified in the score, so that each filter can be heard on its own
  ; from the same instrument.

  sr = 44100
  kr = 4410
  ksmps = 10
  nchnls = 1
  
instr 1
  
  idur     =        p3
  ibegfreq =        p4                              ; beginning of sweep frequency
  iendfreq =        p5                              ; ending of sweep frequency
  ibw      =        p6                              ; bandwidth of filters in Hz
  ifreq    =        p7                              ; frequency of gbuzz that is to be filtered
  iamp     =        p8                              ; amplitude to scale output by
  ires     =        p9                              ; coefficient to scale amount of reson in output
  iresr    =        p10                             ; coefficient to scale amount of resonr in output
  iresz    =        p11                             ; coefficient to scale amount of resonz in output
  
 ; Frequency envelope for reson cutoff
  kfreq    linseg ibegfreq, idur * .5, iendfreq, idur * .5, ibegfreq
  
 ; Amplitude envelope to prevent clicking
  kenv     linseg 0, .1, iamp, idur - .2, iamp, .1, 0
  
 ; Number of harmonics for gbuzz scaled to avoid aliasing
  iharms   =        (sr*.4)/ifreq
  
  asig     gbuzz 1, ifreq, iharms, 1, .9, 1      ; "Sawtooth" waveform
  ain      =        kenv * asig                     ; output scaled by amp envelope
  ares     reson ain, kfreq, ibw, 1
  aresr    resonr ain, kfreq, ibw, 1
  aresz    resonz ain, kfreq, ibw, 1
  
           out ares * ires + aresr * iresr + aresz * iresz
  
endin


</CsInstruments>
<CsScore>

/* Written by Sean Costello */
f1 0 8192 9 1 1 .25                               ; cosine table for gbuzz generator
  
i1  0 10 1 3000 200 100 4000 1 0 0                ; reson  output with bw = 200
i1 10 10 1 3000 200 100 4000 0 1 0                ; resonr output with bw = 200
i1 20 10 1 3000 200 100 4000 0 0 1                ; resonz output with bw = 200
i1 30 10 1 3000  50 200 8000 1 0 0                ; reson  output with bw = 50
i1 40 10 1 3000  50 200 8000 0 1 0                ; resonr output with bw = 50
i1 50 10 1 3000  50 200 8000 0 0 1                ; resonz output with bw = 50
e


</CsScore>
</CsoundSynthesizer>
