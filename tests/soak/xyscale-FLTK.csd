<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
</CsOptions>
<CsInstruments>

ksmps=128
nchnls=2

giwidth = 400
giheight = 300
FLpanel "FLmouse", giwidth, giheight, 10, 10
FLpanelEnd

FLrun

0dbfs = 1

instr 1
  ; We define four chords for bottom-left, bottom-right, top-left and top-right
  ; Use the mouse to interpolate between them
  ibl[] fillarray ntom:i("4C"), ntom:i("4Eb"), ntom:i("4G")
  itl[] fillarray ntom:i("4E"), ntom:i("4G#"), ntom:i("4B")
  ibr[] fillarray ntom:i("4G"), ntom:i("4A"), ntom:i("4B")
  itr[] fillarray ntom:i("4Eb"), ntom:i("4Eb+"), ntom:i("4F")
 
  kmousex, kmousey, kb1, kb2, kb3    FLmouse 2
  kx = limit(kmousex/giwidth, 0, 1)
  ky = 1 - limit(kmousey/giheight, 0, 1)

  printf "x: %f   y: %f \n", changed(kx, ky), kx, ky

  iamp = 0.1
  a0 oscili iamp, mtof(xyscale(kx, ky, ibl[0], itl[0], ibr[0], itr[0]))
  a1 oscili iamp, mtof(xyscale(kx, ky, ibl[1], itl[1], ibr[1], itr[1]))
  a2 oscili iamp, mtof(xyscale(kx, ky, ibl[2], itl[2], ibr[2], itr[2]))
  aout = sum(a0, a1, a2)
  outs aout, aout  
endin

</CsInstruments>
<CsScore>
i 1 0 120

</CsScore>
</CsoundSynthesizer>