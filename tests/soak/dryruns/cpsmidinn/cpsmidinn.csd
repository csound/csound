<CsoundSynthesizer>
; Prints a table showing the equivalents of all Midi
; note numbers from 0-127 in cycles-per-second,
; octave.decimal, and octave.pitchclass units.

<CsInstruments>

instr 1
  ; i-time loop to print conversion table
  imidiNN =   0
  loop1:
    icps  = cpsmidinn(imidiNN)
    ioct  = octmidinn(imidiNN)
    ipch  = pchmidinn(imidiNN)

    print   imidiNN, icps, ioct, ipch

    imidiNN = imidiNN + 1
  if (imidiNN < 128) igoto loop1
endin

instr 2
  ; test k-rate converters
  kMiddleC  =   60
  kcps  = cpsmidinn(kMiddleC)
  koct  = octmidinn(kMiddleC)
  kpch  = pchmidinn(kMiddleC)

endin

</CsInstruments>
<CsScore>
i1 0 0
i2 0 0.1
e

</CsScore>
</CsoundSynthesizer>
