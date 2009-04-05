<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
nchnls =        1

instr 1  
  i1  wiiconnect

;;;      wiirange   20, 0, 100
  kb  wiidata    27
  k1  wiidata    0
  kx  wiidata    20
  ky  wiidata    21
;;  kf  wiidata    26
  knx wiidata    30
  kny wiidata    31
      printks    "%d: %x %f %f [%f %f]\n", 0, kb, k1, kx, ky, knx, kny
endin

</CsInstruments>

<CsScore>

; run for 30 secs
i1 0 30 

</CsScore>

</CsoundSynthesizer>
