<CsoundSynthesizer>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1
; create a PVOC-EX (*.pvx) file with PVANAL first
idur  filelen   "kickroll.pvx"		;find duration of (stereo) analysis file
kpos  line      0,p3,idur		;to ensure we process whole file
fsigr pvsfread  kpos,"kickroll.pvx", 1	;create fsig from right channel
aout  pvsynth	fsigr			;resynthesise it
      outs	aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 10
i 1 11 1
e
</CsScore>
</CsoundSynthesizer>
