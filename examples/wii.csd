<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
nchnls =        1

instr 1  
  i1  wiiconnect 0
      print      i1
      wiirange   20, 0, 100
;;  k1  wiidata    0
;;  kx  wiidata    17
;;  ky  wiidata    18
;;  kz  wiidata    19
;;      printks    "%d %f %f %f\n", 0, k1, kx, ky, kz
     k1  wiidata    -2 
endin

</CsInstruments>

<CsScore>

; run for 30 secs
i1 0 30 

</CsScore>

</CsoundSynthesizer>
