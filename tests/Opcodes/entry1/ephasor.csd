<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr=44100
ksmps=64
nchnls=2
0dbfs = 1

instr 1

iamp = p4
ifr = p5
kfr expon ifr,p3,2*ifr

kfc expon 2000,p3, 4000
kq = 10
kbw = kfc/kq
kR = 1 - $M_PI*(kbw/sr)

k1 = kfc/kfr
kn = int(k1)
k1 = k1 - kn

amod,aph ephasor kfr,kR
aosc1 table aph*kn,-1,1,0,1
aosc2 table aph*(kn+1),-1,1,0,1

asig = iamp*(aosc1*(1 - k1) + aosc2*k1)*amod
  outs asig, asig
  
endin

</CsInstruments>
<CsScore>

i1 0 10 0.5 220

e
</CsScore>
</CsoundSynthesizer>


