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

cnt:k init 0
while cnt < len do
  swp shiftin diskin:a("sweep.wav")
  cnt += ksmps
od
leni:i = filelen("rev.wav")*sr
inp:k[] init leni+ksmps
cnt:k = 0
while cnt < leni do
  inp shiftin diskin:a("rev.wav")
  cnt += ksmps
od

  outp:k[] deconv inp[:leni-1], swp[:len-1]
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
