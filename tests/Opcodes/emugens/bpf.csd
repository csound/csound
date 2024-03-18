<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

; Example for opcode bpf

/*

  bpf stands for Break Point Function

  Given an x value and a series of pairs (x, y), it returns
  the corresponding y value in the linear curve defined by the
  pairs

  It works both at i- and k- time

  ky    bpf kx,    kx0, ky0, kx1, ky1, kx2, ky2, ...
  kys[] bpf kxs[], kx0, ky0, kx1, ky1, kx2, ky2, ...

  NB: x values must be ordered (kx0 < kx1 < kx2 etc)

  See also: bpfcos, linlin, lincos
    
*/

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

instr 1
  kx line -1, p3, 2.5
  ky bpf kx,        \
         0,    0,   \
         1.01, 10,  \
         2,    0.5, \
         2.5,  -1
  if metro(10) == 1 then
    printsk "kx: %f   ky: %f \n", kx, ky
  endif
endin

instr 2
  ; test i-time
  ix = 1.2
  iy bpf ix, 0,0, 0.5,5, 1,10, 1.5,15, 2,20, 2.5,25, 3,30
  prints "iy: %f", iy
  turnoff
endin

instr 3
  ; bpf also works with arrays
  kx[] fillarray 0, 0.15, 0.25, 0.35, 0.45, 0.55, 0.6
  ky[] bpf kx, 0,0, 0.1,10, 0.2,20, 0.3,30, 0.4,40, 0.5,50
  printarray ky, 1, "", "ky="
  turnoff
endin

instr 4
  ; bpf as an envelope generator, like linsegb but driven by external phase
  ; bpf + rms can also be used as compressor

  atime linseg 0, p3*0.62, p3, p3*0.38, 0
  aenv = bpf(atime, 0,0, 0.1,1, 0.5, 0.2) ^ 2
  kbw  = bpf(timeinsts(), 0, 0, p3*0.62, 1) ^ 3
  asig = (beosc(1000, kbw, -1, rnd(6.28)) + beosc(1012, kbw, -1, rnd(6.28))) * 0.3
  kratio bpf dbamp(rms:k(asig)), -12, 1, -6, 0.4, -3, 1/100
  asig *= aenv * interp(lagud(kratio, 0.01, 0.1))
  outs asig, asig
endin
    
</CsInstruments>
<CsScore>
; i 1 0 3 
; i 2 0 -1
; i 3 0 -1
i 4 0 3

</CsScore>
</CsoundSynthesizer>
