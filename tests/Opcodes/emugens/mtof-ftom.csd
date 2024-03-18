<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0
A4 = 440

instr 1
  kfreq = mtof(69)
  printks2 "midi 69   -> %f\n", kfreq
  
  kmidi = ftom(442)
  printks2 "freq 442  -> %f\n", kmidi
  
  kmidi = ftom(442,1)
  printks2 "freq 442  -> %f rounded\n", kmidi
  
  kfreq = mtof(kmidi)
  printks "midi %f -> %f\n", 1, kmidi, kfreq
  
  imidi = ftom:i(440)
  print imidi
  
  ifreq = mtof:i(60)
  print ifreq
  
  turnoff
endin

instr 2
  imidis0[] fillarray 60, 62, 64, 69
  ifreqs0[] mtof imidis0
  printarray ifreqs0, "", "ifreqs0"
  
  kfreqs[] fillarray 220, 440, 880
  kmidis[] ftom kfreqs
  puts "kfreqs", 1
  printarray kmidis, 1, "%.2f", "kmidis"
  turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
i 2 0 1
</CsScore>
</CsoundSynthesizer>
