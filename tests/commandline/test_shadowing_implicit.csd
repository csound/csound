<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode image,i,i
  i1 xin
  xout i1*5
endop

instr 1
imag = 1
imag *= 2
atone = 0
atone *= 2
ia = image(2)
image = -2
image -= 4
print(imag,ia,image)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
