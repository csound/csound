<CsoundSynthesizer>
<CsOptions>
-n -d -v
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 1
0dbfs = 1

instr 1
  ; p default (1) via lentab
  kArr[] genarray_i 0, 3
  kLen  lentab kArr     ; omit 'p'
  printf "lentab (p default=1) -> %d\n", 0, kLen

  ; o default (0) via lfo, phasor
  kLFO  lfo  1, 1         ; omit 'o'
  kPhs  phasor 1          ; omit 'o'
  printf "lfo(o=0)->%.3f  phasor(o=0)->%.3f\n", 0, kLFO, kPhs

  ; j default (-1) via madsr and strsub (length)
  kEnv  madsr 0.01, 0.02, 0.5, 0.03  ; omit 'j'
  Ssub  strsub "hello", 1           ; omit 'j' (length)
  printf "madsr(j=-1) first k=%.3f  strsub(j=-1)='%s'\n", 0, kEnv, Ssub

  ; o default (0) with complex (3rd arg optional)
  C:Complex  complex 1, 2            ; omit 'o'
  ; 'C' not printed (Complex), but triggers defaulting path

  ; h default (127) via veloc: use defaults safely (no MIDI needed)
  ; veloc intypes=oh => optional h
  iVel  veloc   0                    ; omit 'h'
  printf "veloc(h=127) -> %d\n", 0, iVel

endin
</CsInstruments>
<CsScore>
i1 0 0.01
f0 1
</CsScore>
</CsoundSynthesizer>

