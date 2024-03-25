<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

; Show some conversions, both at i- and at k-time

instr 1
  imidi = pchtom(8.09)
  print imidi

  kidx init 0
	
  kpch = 8 + kidx / 100
  kmidi = pchtom(kpch)
  kidx += 1
  printf "kpch: %f    kmidi: %f\n", kidx+1, kpch, kmidi

  if kidx >= 12 then
    turnoff
  endif

endin

</CsInstruments>
<CsScore>
i 1 0 1

</CsScore>
</CsoundSynthesizer>

