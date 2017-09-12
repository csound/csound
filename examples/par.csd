<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 300
nchnls = 1

instr  1
  kamp = 20000
  kcps = p4
  icps = 100
  ifn = 0
  imeth = 1

  a1 pluck kamp, kcps, icps, ifn, imeth
  out a1
endin

instr 2
  kamp = 10000
  kcps = p4
  ifn = 1

  a1 oscil kamp, kcps, ifn
  out a1
endin
</CsInstruments>

<CsScore>
#define BAR #2#
#define FOO #[$BAR + 1]#
f 1 0 16384 10 1
{ 3 $FOO
i 1 0 0.1 440
i 2 0 0.1 220
i 1 0 0.1 110
i 2 0 0.1 330
}


e
</CsScore>
</CsoundSynthesizer>
