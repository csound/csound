<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o loopsegp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
ktrig metro 22/8 ; triggers used to generate new direction values
kdir  trandom ktrig,-2.99,2.99
kdir  =       0.5*int(kdir) ; kdir will either -1, -0.5, 0, 0.5 or 1
; kphase - looping pointer
kphase phasor kdir
; a loop sequence of midi note numbers and durations
knote loopsegp  kphase, 40,1,40,0, 43,1,43,0, 49,2,48,0, \
 47,1,47,0, 46,1,46,0, 46,1,47,0, 49,1,49,0, 43,1,43,0, 46,1,46,0, 40,1,39,0    
kmul  rspline 0.1,0.8,0.5,5                         ; modulation of buzz tone
asig  gbuzz   0.2, cpsmidinn(knote), 30, 3, kmul, 1 ; buzz tone
      outs    asig, asig
      
      schedkwhen ktrig,0,0,2,0,0.1 ; play metronome
endin

instr 2 ; metronome
acps  expon   180+rnd(40),p3,50
aamp  expon   0.05+rnd(0.05),p3,0.001
asig  poscil  aamp-0.001,acps,2
      outs    asig,asig
endin

</CsInstruments>
<CsScore>
; cosine wave.
f 1 0 16384 11 1
; sine wave.
f 2 0 16384 10 1

i 1 0 360 0.25

e
</CsScore>
</CsoundSynthesizer>
