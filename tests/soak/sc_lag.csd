<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

instr 1
  ; smooth a krate signal  
  kx = floor(line(0, p3, 10))
  kx2 = sc_lag(kx, 0.1)
  printk2 kx2
endin

instr 2
  ; smooth an audio signal
  kmidi = floor(line(60, p3, 72)/2)*2
  afreq = upsamp(mtof(kmidi))
  afreqsmooth = sc_lag(afreq, 1)
  a1 = oscili(1, afreq)
  a2 = oscili(1, afreqsmooth)
  outch 1, a1
  outch 2, a2 
endin

</CsInstruments>
<CsScore>
i 1 0 5
i 2 0 10

</CsScore>
</CsoundSynthesizer>
