<CsoundSynthesizer>
<CsOptions>
</CsOptions>
; ==============================================
<CsInstruments>

sr	=	48000
ksmps	=	1
nchnls	=	2
0dbfs	=	1

;; test instruments to demo filter cutoff sweep with high resonance

instr 1	

asig = vco2(0.5, cps2pch(6.00, 12))
asig = K35_lpf(asig, expseg:a(10000, p3, 30), 9.9, 0, 1)
asig *= 0.25
asig  = limit(asig, -1.0, 1.0)

outc(asig, asig)

endin


instr 2	

asig = vco2(0.5, cps2pch(6.00, 12))
asig = K35_lpf(asig, expseg:k(10000, p3, 30), 9.9, 0, 1)
asig *= 0.25
asig  = limit(asig, -1.0, 1.0)

outc(asig, asig)

endin

instr 3	

asig = vco2(0.5, cps2pch(6.00, 12))
asig = K35_hpf(asig, expseg:a(10000, p3, 30), 9.9, 0, 1)
asig *= 0.25
asig  = limit(asig, -1.0, 1.0)

outc(asig, asig)

endin


instr 4	

asig = vco2(0.5, cps2pch(6.00, 12))
asig = K35_hpf(asig, expseg:k(10000, p3, 30), 9.9, 0, 1)
asig *= 0.25
asig  = limit(asig, -1.0, 1.0)

outc(asig, asig)

endin

;; beat instruments

instr ms20_drum

  ipch = cps2pch(p4, 12)
  iamp = ampdbfs(p5)
  aenv = expseg:a(10000, 0.05, ipch, p3 - .05, ipch)

  asig = rand:a(-1.0, 1.0)
  asig = K35_hpf(asig, 60, 7, 1, 1)
  asig = K35_lpf(asig, aenv, 9.8, 1, 1)

  asig = tanh(asig * 16)

  asig *= expon(iamp, p3, 0.0001)

  outc(asig, asig)

endin

instr ms20_bass 
  ipch = cps2pch(p4, 12)
  iamp = ampdbfs(p5)
  aenv = expseg(1000, 0.1, ipch * 2, p3 - .05, ipch * 2)

  asig = vco2(1.0, ipch)
  asig = K35_hpf(asig, ipch, 5, 0, 1)
  asig = K35_lpf(asig, aenv, 8, 0, 1)

  asig *= expon:a(iamp, p3, 0.0001) * 0.8

  outc(asig, asig)
endin

;; perf code

gktempo init 122

opcode beat_dur,i,0
  xout 60 / i(gktempo) 
endop

instr bass_player
  idur = beat_dur() / int(random(1,3)) 
  ipch = 6.00 + int(random(1,3)) + int(random(1,3)) / 100

  schedule("ms20_bass", 0, idur, ipch, -11) 

  if(p2 < 37.5) then
    schedule("bass_player", idur, 0.1)
  endif
  turnoff
endin

instr beat_player 
  istep_total = p4 
  istep = istep_total % 16

  if(istep % 4 == 0) then
    ipch = ((istep_total % 128) < 112) ? 4.00 : 8.00
    iamp = (istep == 0)  ? -9 : -12
    schedule("ms20_drum", 0, 0.5, ipch, iamp)
  endif

  schedule("ms20_drum", 0, 0.125, 14.00, 
           (istep % 4 == 0) ? -12 : -18)

  if(p2 < 37.5) then
    schedule("beat_player", beat_dur() / 4, 0.1, istep_total + 1)
  endif
  turnoff
endin

;; start play of beats

instr start_beats
  schedule("beat_player", 0, 0.1, 0)
  schedule("bass_player", 0, 0.1)
endin


</CsInstruments>
; ==============================================
<CsScore>
i1 0 5.0
i2 5 5.0
i3 10 5.0
i4 15 5.0

i "start_beats" 22 0.5 0

</CsScore>
</CsoundSynthesizer>

