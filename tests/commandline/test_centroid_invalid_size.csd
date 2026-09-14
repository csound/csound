<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#ifndef SIZE
#define SIZE #0#
#endif
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
instr 1
 aInput = 0
 kCentroid centroid aInput, 1, $SIZE
endin
instr 2
 iBins[] fillarray 1
 iCentroid centroid iBins
endin
instr 3
 kBins[] fillarray 1
 kCentroid centroid kBins
endin
; Validate the current array size, including after reinitialization.
instr 4
 kCycle init 0
 kLength init 2
 if kCycle == 1 then
  kLength = 1
  reinit BINS
 endif
BINS:
 kBins[] init i(kLength)
 rireturn
 kCentroid centroid kBins
 kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
i 3 0 .01
i 4 0 .01
e
</CsScore>
</CsoundSynthesizer>
