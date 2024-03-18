<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac     ;      -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o hvs2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; updated example by Stefano Cucchi and Menno Knevel (original creators unknown)

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr	1

inumLinesX init	4 ; number of LINES - matrix -  16 POINTS
inumLinesY init	4 ; numeber of COLOUMNS
inumParms	init	3 ; 3 PARAMETERS  every ponint of the matrix - take a look at iSnapTab
iOutTab	ftgen	5,0,8, -2,      0 ; 
iPosTab	ftgen	6,0,32, -2,     3,2,1,0,4,5,6,7,8,9,10, 11, 15, 14, 13, 12
iSnapTab	ftgen	8,0,64, -2,       1,1,1,     2,0,0,    3,2,0,    2,2,2,    5,2,1,    2,3,4,    6,1,7,      0,0,0, \
                                          1,3,5,     1,4,4,    1,5,8,    1,1,5,    4,3,2,    3,4,5,    7,6,5,      7,8,9

kx linseg 0, p3*0.3, 1, p3*0.3, 0, p3*0.4, 1
ky linseg 1, p3, 0


;     kx,  ky, inumParms,  inumlinesX,  inumlinesY,  iOutTab,  iPosTab,  iSnapTab 
hvs2  kx,  ky, inumParms,  inumLinesX,  inumLinesY,  iOutTab,  iPosTab,  iSnapTab  

k0  tab    0, 5 ; init first value of 3 of iSnapTab
k1  tab    1, 5 ; init  second value of 3 of iSnapTab
k2  tab    2, 5 ; init third value of 3 of iSnapTab

printk2 k0, 10
printk2 k1, 10
printk2 k2, 10

; Make parameters of synthesis depend on the table values produced by hvs
; k0 = first value of group of three in iSnapTab
; k1 = second value of group of three in iSnapTab
; k2 = first value of group of three in iSnapTab

ares1 fof 0.2, k0*100 + 50, k1*100 + 200, 0, k2 * 10 + 50, 0.003, 0.02, 0.007, 20, \
      1, 2, p3
ares2 fof 0.2, k1*100 + 50, k2*100 + 200, 0, k0 * 10 + 50, 0.003, 0.02, 0.007, 20, \
      1, 2, p3
kdeclik linseg 0, 0.3, 1, p3-0.6, 1, 0.3, 0
outs ares1*kdeclik, ares2*kdeclik
	endin

</CsInstruments>
<CsScore>

f 1 0 1024 10 1  ;Sine wave
f 2 0 1024 19 0.5 0.5 270 0.5  ;Grain envelope table

i1 0 10
e
</CsScore>
</CsoundSynthesizer>
