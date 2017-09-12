<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n ;no sound output
</CsOptions>
<CsInstruments>

sr=48000
ksmps=1
nchnls=2

; this test .csd copies image.png into a new file 'imageout.png'

giimage1 imageload "image.png"
giimagew, giimageh imagesize giimage1
giimage2 imagecreate giimagew,giimageh

    instr 1

kndx = 0
kx_ linseg 0, p3, 1
;print imagewidth and imageheigth of image.png
prints "imagewidth = %f pixels, imageheigth = %f pixels\\n", giimagew, giimageh

myloop:
ky_ = kndx/(giimageh)
kr_, kg_, kb_ imagegetpixel giimage1, kx_, ky_
imagesetpixel giimage2, kx_, ky_, kr_, kg_, kb_
loop_lt kndx, 0.5, giimageh, myloop
    endin

    instr 2

imagesave giimage2, "imageout.png"
    endin

    instr 3
imagefree giimage1
imagefree giimage2
    endin

</CsInstruments>
<CsScore>

i1 1 1
i2 2 1
i3 3 1
e

</CsScore>
</CsoundSynthesizer>
 
