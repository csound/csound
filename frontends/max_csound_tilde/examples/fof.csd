<CsoundSynthesizer>
<CsInstruments>
sr      =  44100
ksmps   =  8
nchnls  =  2

0dbfs = 1
zakinit 4,4

giSine ftgen 1, 0, 16385, 10, 1              ; Generate a sine wave table.
giRise ftgen 2, 0, 8193, 19, .5, .5, 270, .5 ; Rise function for fof.

; These global vars define the base table #'s for Ratio, Amp, and BandWidth
giRatioBase init 10
giAmpBase   init 20
giBW_Base   init 30

giRatioA ftgen 10, 0, 8, 17, 0, 1, 1, 1.7333, 2, 3.75,   3, 4.0833, 4, 4.5833
giRatioE ftgen 11, 0, 8, 17, 0, 1, 1, 4.05,   2, 6,      3, 7,      4, 7.75
giRatioI ftgen 12, 0, 8, 17, 0, 1, 1, 7,      2, 10.4,   3, 12.2,   4, 13.36
giRatioO ftgen 13, 0, 8, 17, 0, 1, 1, 1.7142, 2, 6.8571, 3, 7.6428, 4, 8.4285

giAmpA   ftgen 20, 0, 8, 17, 0, 1, 1, .4467, 2, .3548, 3, .3548, 4, .1
giAmpE   ftgen 21, 0, 8, 17, 0, 1, 1, .2512, 2, .3548, 3, .2512, 4, .1259
giAmpI   ftgen 22, 0, 8, 17, 0, 1, 1, .0316, 2, .1585, 3, .0794, 4, .039811
giAmpO   ftgen 23, 0, 8, 17, 0, 1, 1, .1,    2, .0251, 3, .0398, 4, .015849

giBW_A   ftgen 30, 0, 8, 17, 0, 60, 1, 70, 2, 110, 3, 120, 4, 130
giBW_E   ftgen 31, 0, 8, 17, 0, 40, 1, 80, 2, 100, 3, 120, 4, 120
giBW_I   ftgen 32, 0, 8, 17, 0, 60, 1, 90, 2, 100, 3, 120, 4, 120
giBW_O   ftgen 33, 0, 8, 17, 0, 40, 1, 80, 2, 100, 3, 120, 4, 120

#define LFO_TABLE_BASE  # 150 #
giLFOSine        ftgen 150, 0, 4096, 10, 1
giLFOTriangleBi  ftgen 151, 0, 4096, 7, 0, 1024, 1, 1024, 0, 1024, -1, 1024, 0
giLFOSquareBi    ftgen 152, 0, 4096, 7, 1, 2048, 1, 0, -1, 2048, -1
giLFOSquareUni   ftgen 153, 0, 4096, 7, 1, 2048, 1, 0, 0, 2048, 0
giLFOUpUni       ftgen 154, 0, 4096, 7, 0, 4096, 1
giLFODownUni     ftgen 155, 0, 4096, 7, 1, 4096, 0

opcode mtofK, k, k
    kIn xin
    kOut = (440.0*exp(log(2.0)*(kIn-69.0)/12.0))
    xout kOut
endop

instr 1
    kAmp     chnget "amp"
    kFund    chnget "fund"
    kForm    chnget "form"
    kOct     chnget "oct"
    kTab     chnget "tab"
    kPort    chnget "port"
    kLFOfrq  chnget "lfoFrq"
    kLFOamp  chnget "lfoAmp"
    kLFOtyp  chnget "lfoTyp"    

	kForm = (kForm^2) * 10000

    kAmp  portk kAmp, kPort
    kFund portk kFund, kPort
    kForm portk kForm, kPort
    kOct  portk kOct, kPort

    aSync1 = 0
    kPhase1 = 0
    aLFO oscilikts kLFOamp, kLFOfrq, kLFOtyp + $LFO_TABLE_BASE, aSync1, kPhase1
    kLFO downsamp aLFO
    kLFO port kLFO, .002
    kFund = kFund + kLFO

    kFund mtofK kFund
    kFund limit kFund, 10, 10000

    kTab limit kTab, 0, 3

    kR0 tableikt 0, giRatioBase + kTab
    kR1 tableikt 1, giRatioBase + kTab
    kR2 tableikt 2, giRatioBase + kTab
    kR3 tableikt 3, giRatioBase + kTab
    kR4 tableikt 4, giRatioBase + kTab

    kA0 tableikt 0, giAmpBase + kTab
    kA1 tableikt 1, giAmpBase + kTab
    kA2 tableikt 2, giAmpBase + kTab
    kA3 tableikt 3, giAmpBase + kTab
    kA4 tableikt 4, giAmpBase + kTab
    
    kB0 tableikt 0, giBW_Base + kTab
    kB1 tableikt 1, giBW_Base + kTab
    kB2 tableikt 2, giBW_Base + kTab
    kB3 tableikt 3, giBW_Base + kTab
    kB4 tableikt 4, giBW_Base + kTab

    kR0 portk kR0, kPort
    kR1 portk kR1, kPort
    kR2 portk kR2, kPort
    kR3 portk kR3, kPort
    kR4 portk kR4, kPort

    kA0 portk kA0, kPort
    kA1 portk kA1, kPort
    kA2 portk kA2, kPort
    kA3 portk kA3, kPort
    kA4 portk kA4, kPort

    kB0 portk kB0, kPort
    kB1 portk kB1, kPort
    kB2 portk kB2, kPort
    kB3 portk kB3, kPort
    kB4 portk kB4, kPort

    ;  FOF amp  fund   form       oct   band  ris   dur   dec  iolaps  ifna    ifnb    idur
    a1 fof kA0, kFund, kForm*kR0, kOct, kB0, .003, .017, .007, 2000,   giSine, giRise, 84600
    a2 fof kA1, kFund, kForm*kR1, kOct, kB1, .003, .017, .007, 2000,   giSine, giRise, 84600
    a3 fof kA2, kFund, kForm*kR2, kOct, kB2, .003, .017, .007, 2000,   giSine, giRise, 84600
    a4 fof kA3, kFund, kForm*kR3, kOct, kB3, .003, .017, .007, 2000,   giSine, giRise, 84600
    a5 fof kA4, kFund, kForm*kR4, kOct, kB4, .003, .017, .007, 2000,   giSine, giRise, 84600

    aOsc oscil .125, kFund, giSine

    aSig = (a1 + a2 + a3 + a4 + a5 + aOsc) * .1
    aSig dcblock aSig   

    aSig eqfil aSig, 400, 800, .75

    zawm aSig * kAmp, 1
endin

; Reverb instrument.
instr 2
    aSig zar 1

    kRvbTime chnget "time"
    kHiAttn  chnget "attn"
    kRvb     chnget "rvb"

    aRvb, aCrap freeverb aSig, aSig, kRvbTime, kHiAttn
    aOut = aSig + (aRvb * kRvb)

    outch 1, aOut, 2, aOut
    
    zacl 0, 4
endin

</CsInstruments>
<CsScore>
f0 86400
i 1 0 -1
i 2 0 -1
e
</CsScore>
</CsoundSynthesizer>
