<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

; Example for opcode bpfcos

/*

  bpf stands for Break Point Function

  Given an x value and a series of pairs (x, y), it returns
  the corresponding y value in the half cosine curve defined by the
  pairs

  It works both at i- and k- time

  ky     bpfcos kx,    kx0, ky0, kx1, ky1, kx2, ky2, ...
  kys[]  bpfcos kxs[], kx0, ky0, kx1, ky1, kx2, ky2, ...
  ky     bpfcos kx, kxs[], kys[]
  ky, kz bpfcos kx, kxs[], kys[], kzs[]

  NB: x values must be ordered (kx0 < kx1 < kx2 etc)

  See also: bpf, linlin, lincos
  
*/
sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1
    
instr 1
  kx line -1, p3, 2.5
  ky bpfcos kx,        \
            0,    0,   \
            1.01, 10,  \
            2,    0.5, \
            2.5,  -1
  printks "kx: %f   ky: %f \n", 0.1, kx, ky
endin

instr 2
  ; test i-time
  ix = 1.2
  iy bpfcos ix, 0,0, 0.5,5, 1,10, 1.5,15, 2,20, 2.5,25, 3,30
  print iy
  turnoff
endin

instr 3
  ; bpfcos also works with arrays. For each kx value in kxs, 
  ; calculate the corresponding ky 
  kxs[] fillarray 0, 0.15, 0.25, 0.35, 0.45, 0.55, 0.6
  kys[] bpfcos kxs, 0,0, 0.1,10, 0.2,20, 0.3,30, 0.4,40, 0.5,50
  printarray kys, 1, "", "kys="
  turnoff
endin

instr 4
  ; bpfcos is useful to implement envelopes with ease-in/out shape
  kpitch bpfcos timeinsts(), 0, 60, 2, 61, 3, 65, 3.5, 60
  a0 oscili 0.5, mtof(kpitch)
  kenv0 linseg 0, 0.5, 1, p3-1, 1, 0.5, 0
  kenv bpfcos kenv0, 0, 0, 0.5, 0.25, 1, 0.7
  a0 *= interp(kenv)
  outch 1, a0
endin

instr 5
  ; arrays can also be used to define the points of a break-point-function
  ; multiple arrays can be used simultaneously
  ; In this case, we define a line and for each point in the line a 
  ; corresponding pitch (midinote) and amplitude
  ; NB: kTimes uses absolute times
  kTimes[] fillarray    0,  1, 1.5,  2,   3,  5
  kPitches[] fillarray 60, 65,  64, 69,  60, 61
  kAmps[] fillarray     0,  1, 0.1,  1, 0.1,  1
  ; play the envelopes at half speed
  kpitch, kamp bpfcos timeinsts()*0.5, kTimes, kPitches, kAmps
  aout oscili a(kamp), a(mtof:k(kpitch))
  ; declick
  aout *= linsegr(0, 0.1, 1, 0.1, 0)
  outch 1, aout
endin

</CsInstruments>
<CsScore>
; i 1 1 3 
; i 2 0 -1
; i 3 0 -1
; i 4 0 5
i 5 0 10
e 12
</CsScore>
</CsoundSynthesizer>
