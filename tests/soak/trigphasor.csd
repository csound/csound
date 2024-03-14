<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1.0

gisnd ftgen 0, 0, 0, -1, "finneganswake1.flac", 0, 0, 0

instr 1
  ; use trigphasor to read a soundfile
  inumsamps = nsamp(gisnd)
  irate = 1  ; play at original speed
  ktrig metro 0.25
  aphase trigphasor ktrig, irate, 0, inumsamps
  asig table3 aphase, gisnd
  asig *= linsegr:a(0, 0.01, 1, 0.01, 0)
  outch 1, asig
endin
	
</CsInstruments>
<CsScore>
i 1 0 20
f0 3600

</CsScore>
</CsoundSynthesizer>
