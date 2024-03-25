<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen17.wav -W ;;; for file output any platform
; By Stefano Cucchi & Menno Knevel - 2021
</CsOptions>
<CsInstruments>

sr     =    44100
ksmps  =    32
nchnls =    2
0dbfs =     1

instr 1 
gisqre ftgen 2, 0, 16384, 10, 1, 0 , .33, 0, .2 , 0, .14, 0 , .11, 0, .09 ;odd harmonics

knoteleft oscil 1, 0.5, 10       ; index to table 10 - gen 17 - every 2 seconds reads all the values in the table n. 10
printks2 "note left  = %d\n", knoteleft
knoteright oscil 1, 1, 10        ; index to table 10 - gen 17 - every  second reads all the values in the table n. 10
printks2 "note right = %d\n", knoteright

ixmode =    1
ixoff =     0
iwrap =     1
aphasor1 phasor knoteleft        ; the values in table 10 become the frequency of the oscillator
asig1 tablei aphasor1, gisqre, ixmode, ixoff, iwrap ; oscillator generating sound in the left channel (table 10 every 2 seconds)
aphasor2 phasor knoteright       ; the values in table 10 become the frequency of the oscillator
asig2 tablei aphasor2, gisqre, ixmode, ixoff, iwrap ; oscillator generating sound in the right channel (table 10 every second)
kgenenv linseg 0, 0.3, 0.4, p3-0.6, 0.4, 0.3, 0     ; envelope
         outs asig1 * kgenenv, asig2 * kgenenv
endin

</CsInstruments>
<CsScore>
; table with gen17 - At point 0 pitch 300 Hz, at point 12 pitch 350 Hz, etc...
f  10  0  128  -17   0  300   12  350   20  400   41  434   48  563  67  589   72  632   79  678  100 712 120 789

i1 0 10 
e
</CsScore>
</CsoundSynthesizer>
