<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen15.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;example from the Csound Book, page 85
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idur  = p3
iamp  = p4
ifrq  = cpspch(p5)        ;pitch
iswp1 = p6
iswp2 = p7
kswp  line   iswp1, p3, iswp2      ;amplitude sweep values
acosi oscili kswp*.5, ifrq, 2      ;f2=cosine wave
asine oscili kswp, ifrq, 1         ;f1=sine wave
atab1 tablei acosi, 33, 1, .5      ;tables a1 to GEN13
atab2 tablei acosi, 34, 1, .5      ;tables a1 to GEN14
knrm1 tablei kswp, 35, 1           ;normalizing f35
knrm2 tablei kswp, 36, 1           ;normalizing f36
anrm1 = atab1*knrm1                ;normalize GEN13 signal
anrm2 = atab2*knrm2*asine          ;normalize GEN14 signal
amix  = anrm1+anrm2                ;mix GEN13 and GEN14
kenv  expseg .001, idur*.1, iamp, idur*.1, iamp*.8, idur*.8, .001  
asig  = amix*kenv  
      outs   asig, asig

endin

</CsInstruments>
<CsScore>
f 1 0 8193 10  1        ;sine wave
f 2 0 8193  9  1 1 90   ;cosine wave

; Note that all the f33 tables in the following sections are defined with p4=-15,
; which means that tables 33 and 34 will not be normalized. Thus if we display
; tables when running this example, we'll get correct diagrams even if one table
; has very small values instead of 0 values, due to cpu approximations in processing
; sin(180), as in sections 2, 4, and 5. This has no consequence on the audio result,
; because of the use of amp normalization (tables 35 and 36).

f 33 0 8193 -15 1 1 1 0 1 180 .8 45 .6 270 .5 90 .4 225 .2 135 .1 315  ;makes function tables 33 and 34
f 35 0 4097  4  33 1    ;amp normalization for f33
f 36 0 4097  4  34 1    ;amp normalization for f34
i 1 0 5  .6  8.00 0   1
i 1 + .  .6  8.00 1   0
s
;even harmonics with no phase shift, odd harmonics with phase shift
f 33 0 8193 -15 1 1 1 0 1 0 1 180 1 180 1 0 1 0 1 180 1 180 1 0 1 0 1 180 1 180
f 35 0 4097  4  33 1    ;amp normalization for f33
f 36 0 4097  4  34 1    ;amp normalization for f34
i 1 0  5 .6  8.00 0   1
i 1 +  . .6  8.00 1   0
s
;different harmonic strenghts and phases
f 33 0 8193 -15 1 1 1 0 1 0 .9 180 .5 270 .75 90 .4 45 .2 225 .1 0
f 35 0 4097  4  33 1    ;amp normalization for f33
f 36 0 4097  4  34 1    ;amp normalization for f34
i 1 0 5 .6  8.00 0  1
i 1 + . .6  8.00 1  0
s
;lower harmonics no phase shift, upper harmonics with phase shift
f 33 0 8193 -15 1  1  1 0 1 0 .5 0 .9 0 .3 0 .75 0 .2 180 .6 180 .15 180 .5 180 .1 180  
f 35 0 4097  4  33 1    ;amp normalization for f33
f 36 0 4097  4  34 1    ;amp normalization for f34
i 1 0 5 .6  8.00 0   1
i 1 + . .6  8.00 1   0

s
;lower harmonics with phase shift, upper harmonics no phase shift
f 33 0 8193 -15 1 1 1 180 1 180 .5 180 .9 180 .3 180 .75 180 .2 0 .6 0 .15 0 .5 0 .1 0  
f 35 0 4097 4 33 1    ;amp normalization for f33
f 36 0 4097 4 34 1    ;amp normalization for f34
i 1 0 5 .6  8.00 0  1
i 1 + . .6  8.00 1  0
e
</CsScore>
</CsoundSynthesizer>
