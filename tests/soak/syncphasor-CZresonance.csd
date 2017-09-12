<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o syncphasor-CZresonance.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
; by Anthony Kozar. February 2008
; http://www.anthonykozar.net/

; Imitation of the Casio CZ-series synthesizer's "Resonance" waveforms
; using a synced phasor to read a sinusoid table.  The jumps at the sync 
; points are smoothed by multiplying with a windowing function controlled
; by the master phasor.

; Based on information from the Wikipedia article on phase distortion:
; http://en.wikipedia.org/wiki/Phase_distortion_synthesis

; Sawtooth Resonance waveform. Smoothing function is just the inverted
; master phasor.

; The Wikipedia article shows an inverted cosine as the stored waveform,
; which implies that it must be unipolar for the smoothing to work.
; I have substituted a sine wave in the first phrase to keep the output
; bipolar.  The second phrase demonstrates the much "rezzier" sound of the
; bipolar cosine due to discontinuities.

instr 1
  ifreq      =           cpspch(p4)
  initReson  =           p5
  itable     =           p6
  imaxamp    =           10000
  anosync    init        0.0

  kslavecps       line        ifreq * initReson, p3, ifreq
  amaster, async  syncphasor  ifreq, anosync		; pair of phasors
  aslave, async2  syncphasor  kslavecps, async		; slave synced to master
  aosc            tablei      aslave, itable, 1		; use slave phasor to read a (co)sine table
  aout            =           aosc * (1.0 - amaster)	; inverted master smoothes jumps 
  adeclick        linseg      0.0, 0.05, 1.0, p3 - 0.1, 1.0, 0.05, 0.0

                    out         aout * adeclick * imaxamp
endin

; Triangle or Trapezoidal Resonance waveform. Uses a second table to change
; the shape of the smoothing function.  (This is my best guess so far as to
; how these worked). The cosine table works fine with the triangular smoothing
; but we once again need to use a sine table with the trapezoidal smoothing.

; (It might be interesting to be able to vary the "width" of the trapezoid.
; This could be done with the pdhalf opcode).

instr 2
  ifreq      =           cpspch(p4)
  initReson  =           p5
  itable     =           p6
  ismoothtbl =           p7
  imaxamp    =           10000
  anosync    init        0.0

  kslavecps       line        ifreq * initReson, p3, ifreq
  amaster, async  syncphasor  ifreq, anosync		; pair of phasors
  aslave, async2  syncphasor  kslavecps, async		; slave synced to master
  aosc            tablei      aslave, itable, 1		; use slave phasor to read a (co)sine table
  asmooth         tablei      amaster, ismoothtbl, 1	; use master phasor to read smoothing table
  aout            =           aosc * asmooth
  adeclick        linseg      0.0, 0.05, 1.0, p3 - 0.1, 1.0, 0.05, 0.0

  out         aout * adeclick * imaxamp
endin

</CsInstruments>
<CsScore>
f1 0 16385 10  1
f3 0 16385  9  1 1 270			; inverted cosine
f5 0 4097   7  0.0 2048 1.0 2049 0.0	; unipolar triangle
f6 0 4097   7  1.0 2048 1.0 2049 0.0    ; "trapezoid"

; Sawtooth resonance with a sine table
i1 0 1    7.00  5.0  1
i. + 0.5  7.02  4.0
i. + .    7.05  3.0
i. + .    7.07  2.0
i. + .    7.09  1.0
i. + 2    7.06  12.0
f0 6
s

; Sawtooth resonance with a cosine table
i1 0 1    7.00  5.0  3
i. + 0.5  7.02  4.0
i. + .    7.05  3.0
i. + .    7.07  2.0
i. + .    7.09  1.0
i. + 2    7.06  12.0
f0 6
s

; Triangle resonance with a cosine table
i2 0 1    7.00  5.0  3  5
i. + 0.5  7.02  4.0
i. + .    7.05  3.0
i. + .    7.07  2.0
i. + .    7.09  1.0
i. + 2    7.06  12.0
f0 6
s

; Trapezoidal resonance with a sine table
i2 0 1    7.00  5.0  1  6
i. + 0.5  7.02  4.0
i. + .    7.05  3.0
i. + .    7.07  2.0
i. + .    7.09  1.0
i. + 2    7.06  12.0

e

</CsScore>
</CsoundSynthesizer>
