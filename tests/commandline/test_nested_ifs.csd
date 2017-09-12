<CsoundSynthesizer>

<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

instr 1

k1 init 1

if (k1 == 1) then

  if (k1 == 1) then
   kval = 2
  else
   kval = 1
  endif

  printk .1, kval
elseif (k1 == 2) then
  kval = 0
else
  kval = -1
endif

printk .1, kval

endin

</CsInstruments>

<CsScore>
i1 0 .5
</CsScore>

</CsoundSynthesizer>
