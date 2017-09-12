<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

ilast readscratch
      writescratch p2
ilen  readscratch 1
      writescratch p3, 1
      printf_i "last run at %f for %f\n", ilen, ilast, ilen
endin

</CsInstruments>
<CsScore>
i 1 0 1
i 1 2 3
i 1 6 10
e
</CsScore>
</CsoundSynthesizer>

