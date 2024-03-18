<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o pvscross.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel 2021, after an example by joachim heintz 2009

instr 1
  ipermut  = p4                                                  ; 1 = change order of soundfiles
  ifftsize = 1024
  ioverlap = ifftsize / 4
  iwinsize = ifftsize
  iwinshape = 1                                                  ; von-Hann window
  Sfile1 = "oboe.aiff"
  Sfile2 = "finneganswake1.flac"
  ain1   = diskin2:a(Sfile1, .5, 0, 1)
  ain2   = diskin2:a(Sfile2, 1, 0, 1)                             ; the wave plays 4 x faster and higher
  fftin1 = pvsanal(ain1, ifftsize, ioverlap, iwinsize, iwinshape) ; fft-analysis of file 1
  fftin2 = pvsanal(ain2, ifftsize, ioverlap, iwinsize, iwinshape) ; fft-analysis of file 2
  ktrans = linseg(0, p3*.2, 0, p3*.6, p5, p3*.2, p5)              ; transitions
  if ipermut == 1 then
    fcross = pvscross(fftin2, fftin1, ktrans, 1 - ktrans)
  else
    fcross = pvscross(fftin1, fftin2, ktrans, 1 - ktrans)
  endif
  aout = pvsynth(fcross)
  outs(aout, aout)
endin

</CsInstruments>
<CsScore>
; use only first portion of sample (=p5)
i 1 0 12.7   0    0     ; frequencies from the oboe, no transition
i 1 14 12.7  1    0     ; frequencies from the voice, no transition
; transition over total sample (=p5)
i 1 28 12.7  0    1    ; frequencies from the oboe, amplitude transition from voice to oboe
i 1 42 12.7  1    1    ; frequencies from the voice, amplitude transition from oboe to voice
e
</CsScore>
</CsoundSynthesizer>
