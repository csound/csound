<CsTest>
description = "test diskin2 crossfade in rt async mode"
args = []

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null --realtime -d -m128
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

; Offline tests cover the xfade algorithm; this covers the realtime 
; paths (scalar, array, hard wrap and sync) with
; overlapping notes so the worker races init/deinit.

; diskin2: Spath, kpitch, iskiptim, iwrap=0, iformat=0, iwsize=0, ibufsize=, iskipinit=0, isync=0, iend=0

instr 1
  ;          S          p  sk   wr   
  a1 diskin2 "fox.wav", 1, 0.5, 128, 0,0,0,0,0, 1.
  out a1*0.5
endin

instr 2                       
  a1[] diskin2 "fox.wav", 1.5, 0.5, 128, 0,0,0,0,0, 0.7
  out a1[0]*0.5
endin

instr 3
  ; hard wrap
  a1 diskin2 "fox.wav", 1, 0.5, 1, 0,0,0,0,0, .1
  out a1*0.5
endin

instr 4
  ; sync mode
  a1 diskin2 "fox.wav", 1, 0.5, 1000, 0,0,0,0,1, 1.
  out a1*0.5
endin

</CsInstruments>

<CsScore>
i1 0   1.5
i2 0.5 1.5
i1 1.0 1.5
i2 1.5 1.5
i3 2.0 1.5
i4 2.5 1.5
</CsScore>
</CsoundSynthesizer>
