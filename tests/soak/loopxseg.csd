<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o loopxseg.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
kfreq  rspline  0.01,20,0.2,1   ; freq. of loop repetition created by random spline
ktrig  init     0   ; loop restart trigger (not used)
iphase =        0   ; initial phase
; loop of filter cutoff values (oct format). Rescaled further down.
kcfoct loopxseg  kfreq, ktrig, iphase, 1,1,0,0
kenv  linseg   0,0.01,1,p3-5.01,1,5,0
asig  vco2     0.2*kenv,cpsmidinn(48),0
kdep  rspline  5,8,0.2,1  ; filter depth created by a random spline
kcf   port     cpsoct((kcfoct*kdep)+4), 0.001  ; smooth filter changes
asig  moogladder  asig,kcf,rnd(0.6)
aL,aR pan2     asig,rnd(1)
      outs     aL, aR
endin

</CsInstruments>
<CsScore>
i 1  0 60
e
</CsScore>
</CsoundSynthesizer>
