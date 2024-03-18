<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    Silent  MIDI in
-odac
-d
</CsOptions>

<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

instr 1
  aenv linsegr 0, 0.1, 1, 0.1, 0
  asig =  oscili(0.1, 1000)
  asig += oscili(0.1, 1012)
  asig *= aenv
  if lastcycle() == 1 then
    schedulek p1, 0, p3
  endif
  outs asig, asig
endin
  
</CsInstruments>

<CsScore>

i 1 0 0.5
f 0 3600 

</CsScore>

</CsoundSynthesizer>
