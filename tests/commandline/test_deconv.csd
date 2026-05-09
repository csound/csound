<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

instr 1
len:i = filelen("sweep.wav")*sr
tab:i = ftgen(1, 0, len, 2, 0)
swp:k[] init len+ksmps
inp:k[] init len+ksmps
cnt:k init 0
while cnt < len do
  swp shiftin diskin:a("sweep.wav")
  inp shiftin diskin:a("rev.wav")
  cnt += ksmps
od
  outp:k[] deconv inp, swp[:len-1]
  copya2ftab outp, tab
turnoff
endin

instr 2
sig:a diskin2 "fox.wav"
rev:a ftconv sig, 1, 64
     out rev*0.5
endin
</CsInstruments>
<CsScore>
i1 0 1
i2 1 5
</CsScore>
</CsoundSynthesizer>
