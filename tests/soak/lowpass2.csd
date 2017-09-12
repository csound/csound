<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o lowpass2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

/* Written by Sean Costello */
; Orchestra file for resonant filter sweep of a sawtooth-like waveform.
  sr = 44100
  kr = 2205
  ksmps = 20
  nchnls = 1

          instr 1

  idur    =          p3
  ifreq   =          p4
  iamp    =          p5 * .5
  iharms  =          (sr*.4) / ifreq

; Sawtooth-like waveform
  asig    gbuzz 1, ifreq, iharms, 1, .9, 1

; Envelope to control filter cutoff 
  kfreq   linseg 1, idur * 0.5, 5000, idur * 0.5, 1

  afilt   lowpass2 asig,kfreq, 30

; Simple amplitude envelope
  kenv    linseg 0, .1, iamp, idur -.2, iamp, .1, 0 
          out afilt * kenv

          endin


</CsInstruments>
<CsScore>

/* Written by Sean Costello */
f1 0 8192 9 1 1 .25

i1 0 5 100 1000
i1 5 5 200 1000
e


</CsScore>
</CsoundSynthesizer>
