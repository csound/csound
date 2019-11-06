<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

gisaw	ftgen	1, 0, 2048, 10, 1, 0.5, 0.3, 0.25, 0.2	;sawtooth-like

instr 1
  Sfile = "fox.wav"
  asig soundin Sfile
  asyn poscil .6, 150, gisaw		;excitation signal of 150 Hz
  famp pvsanal asig, 1024, 256, 1024, 1	;analyse in signal
  fexc pvsanal asyn, 1024, 256, 1024, 1	;analyse excitation signal
  ftps pvsvoc  famp, fexc, 1, 1		;cross it
  atps pvsynth ftps			;synthesise it
  outs atps, atps
endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
