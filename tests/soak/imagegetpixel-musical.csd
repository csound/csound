<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o imagegetpixel-musical.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

#define TABLEN# 64#
giRtab ftgen 1, 0, $TABLEN, 10, 1
giGtab ftgen 2, 0, $TABLEN, 10, 1
giBtab ftgen 3, 0, $TABLEN, 10, 1

gisin ftgen 11, 0, 1024, 10, 1

giimg imageload "imageOpcode03.png"
giimgH, giimgW imagesize giimg

instr 1
kndx = 0

kyR oscili .5, .15, 11
kyR = (kyR + .5) * .999

kyG oscili .5, .25, 11
kyG = (kyG + .5) * .999

kyB oscili .5, .4, 11
kyB = (kyB + .5) * .999

ilen = 64
loop:
kR, k0, k1 imagegetpixel giimg, kndx / (ilen+1), kyR
k2, kG, k3 imagegetpixel giimg, kndx / (ilen+1), kyG
k4, k5, kB imagegetpixel giimg, kndx / (ilen+1), kyB

kRval = kR * 2 - 1
tabw kRval, kndx, giRtab

kGval = kG * 2 - 1
tabw kGval, kndx, giGtab

kBval = kB * 2 - 1
tabw kBval, kndx, giBtab

if kndx < ilen then
kndx = kndx + 1
kgoto loop
endif

iampgain = .3
kamp madsr 2, 0, 1, 3
kamp = kamp * iampgain
ifrq = cpspch(p4)

aR oscil3 kamp * kyG, ifrq, giRtab
aG oscil3 kamp * kyB * .8, ifrq * 2.01, giGtab
aB oscil3 kamp * kyR * .8, ifrq * 1.99, giBtab

aR butlp aR, 5000
aG butlp aG, 10000
aB butlp aB, 10000

asigL = (aR + aG)*.9     ; green Left channel
asigR = (aR + aB)*.9     ; blue Right channel & red in the middle

adelL comb asigL, 1, .25
asigL = asigL + adelL * .5
asigL dcblock asigL

adelR comb asigR, 1, .3
asigR = asigR + adelR * .5
asigR dcblock asigR

outs asigL, asigR

endin

</CsInstruments>
<CsScore>
i1 0 25 8.00
i. 5 20 8.02
i. 10 10 8.07
i. 15 10 8.09
i. 20 5 8.11

</CsScore>
</CsoundSynthesizer>
