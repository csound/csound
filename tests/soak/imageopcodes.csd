<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n ;no sound output
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

; by Cesare Marilungo 2008
; additions by Menno Knevel 2021
; image opcodes need a black canvas- black = no sound!

giimage1 imageload "imageOpcode01.png"              ; load this image
giimagew, giimageh imagesize giimage1               ; get dimensions of imageOpcode01.png
giimageNEW imagecreate giimagew,giimageh            ; and use those same dimensions for the new image            

instr 1 ; copies imageOpcode01.png and changes it

kndx = 0
kx linseg 0, p3, 1
prints "\nwidth = %d pixels, heigth = %d pixels\n\n", giimagew, giimageh

myloop:
ky = kndx/(giimageh)                                ; y-axis
krd, kgn, kbl imagegetpixel giimage1, kx, ky        ; get pixels from 'old' image
imagesetpixel giimageNEW, kx*.9, ky*.5, krd, kgn, kbl; redesign the image
loop_lt kndx, 0.5, giimageh, myloop
endin

instr 2
imagesave giimageNEW, "imageOUT.png"                ; save this new image
endin

instr 3
imagefree giimage1                                  ; unload images
imagefree giimageNEW
endin

</CsInstruments>
<CsScore>

i1 1 1
i2 2 1
i3 3 .1
e
</CsScore>
</CsoundSynthesizer>
 
