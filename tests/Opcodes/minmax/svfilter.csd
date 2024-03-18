<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac   --limiter=.95 ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o svfilter.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
  
instr 1
  
idur     = p3
ifreq    = p4
iamp     = p5
ilowamp  = p6              ; determines amount of lowpass output in signal
ihighamp = p7              ; determines amount of highpass output in signal
ibandamp = p8              ; determines amount of bandpass output in signal
iq       = p9              ; value of q
  
iharms   =  (sr*.4) / ifreq
  
asig    gbuzz .4, ifreq, iharms, 1, .9, 1               ; Sawtooth-like waveform
kfreq   linseg 1, idur * 0.5, 4000, idur * 0.5, 1       ; Envelope to control filter cutoff
  
alow, ahigh, aband   svfilter asig, kfreq, iq
  
aout1   =         alow * ilowamp
aout2   =         ahigh * ihighamp
aout3   =         aband * ibandamp
asum    =         aout1 + aout2 + aout3
kenv    linseg 0, .1, iamp, idur -.2, iamp, .1, 0       ; Simple amplitude envelope
outs    asum * kenv, asum * kenv
  
endin

</CsInstruments>
<CsScore>
f1 0 8192 9 1 1 .25
  
i1  0 5 100 .1 1 0 0  5  ; lowpass sweep
i1  5 5 200 .1 1 0 0 30  ; lowpass sweep, octave higher, higher q
i1 10 5 100 .1 0 1 0  5  ; highpass sweep
i1 15 5 200 .1 0 1 0 30  ; highpass sweep, octave higher, higher q
i1 20 5 100 .1 0 0 1  5  ; bandpass sweep
i1 25 5 200 .1 0 0 1 30  ; bandpass sweep, octave higher, higher q
i1 30 5 200 .2 .4 .6  0  ; notch sweep - notch formed by combining highpass and lowpass outputs
e
</CsScore>
</CsoundSynthesizer>
