<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
-odac  --limiter=.95 ;;;realtime audio out and limiter
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o imagegetpixel.wav -W ;;; for file output any platform
</CsOptions> 
<CsInstruments> 

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

; by Cesare Marilungo 2008
; additions by Menno Knevel 2021

giimage01 imageload "imageOpcode01.png"                     ; load 2 images - should be 512x512 pixels
giimagew01, giimageh01  imagesize   giimage01
giimage02 imageload "imageOpcode02.png"
giimagew02, giimageh02  imagesize   giimage02

gisine ftgen 1, 0, 8192, 10, 1                              ; sine wave
gifrqs ftgen 2, 0, 512, -5, 1, 512, 10                      ; will hold the frequencies
giamps ftgen 3, 0, 512, 10, 1                               ; ready for amplitude data

instr 1

kindex = 0
icnt = giimageh01                                           ; height of image imageOpcode01.png
kx_ linseg 0, p3, 1                                         ; scans x-axis
kenv linseg 0, p3*.1, .1, p3*.8, .1, p3*.1, 0               ; amplitude envelope 

; Read a column of pixels and store the red values inside the table 'giamps'
loop:
    ky_ = 1-(kindex/giimageh01)                             ; reverses direction of reading
    kred, kgreen, kblue imagegetpixel giimage01, kx_, ky_   ; get the pixel color values at kx_, ky_ 
    tablew kred, kindex, giamps                             ; write the red values inside the table 'giamps'
    kindex = kindex+1
if (kindex < icnt) kgoto loop
; setting amplitudes for each partial according to the image
asig adsynt2 kenv, 110, gisine, gifrqs, giamps, icnt, 2     ; oscillator bank (additive synthesis)
outs asig, asig
endin

instr 2   ; Free memory used by imageOpcode01.png
imagefree giimage01
endin

instr 3

kindex = 0
icnt = giimageh02                                           ; height of image imageOpcode02.png
kx_ linseg 0, p3, 1                                         ; scans x-axis
kenv linseg 0, p3*.2, .02, p3*.4, .02, p3*.2, 0             ; amplitude envelope 
; Read a column of pixels and store the blue values inside the table 'giamps'
loop:
    ky_ = 1-(kindex/giimageh02)                             ; reverses direction of reading
    kred, kgreen, kblue imagegetpixel giimage02, kx_, ky_   ; get the pixel color values at kx_, ky_ 
    tablew kblue, kindex, giamps                            ; write the blue values inside the table 'giamps'
    kindex = kindex+1
if (kindex < icnt) kgoto loop

asig adsynt2 kenv, 100, gisine, gifrqs, giamps, icnt, 2     ; oscillator bank (additive synthesis)
outs asig, asig
endin

instr 4   ; Free memory used by imageOpcode02.png
imagefree giimage02
endin

</CsInstruments> 
<CsScore>
i1  1  3    ; play first image           
i2  4 .1    ; then unload first image 
i3  5  5    ; play second image
i4 11 .1    ; and unload that one     
e
</CsScore>
</CsoundSynthesizer>
