<CsoundSynthesizer>
<CsInstruments>

sr	    =  48000
ksmps	    =  16
nchnls    =  2
0dbfs	    =  1

; Example by Jonathan Murphy 2007

gSinstr2  =  "beep"

instr 1

  ktrig	    metro     1
  if (ktrig == 1) then
    ;Call instrument "printer" once per second
    schedkwhennamed ktrig, 0, 1, gSinstr2, 0, 1

  endif

endin

instr beep
  asig = poscil:a(0.2, 440)
  outs asig, asig
endin

</CsInstruments>
<CsScore>
i1 0 10
e
</CsScore>
</CsoundSynthesizer>
