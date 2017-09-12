<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen12.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;example from the Csound Book, page 87
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

idur    =       p3
iamp    =       p4
icarfrq =       p5
imodfrq =       p6          
aenv    expseg  .01, idur*.1, iamp, idur*.8, iamp*.75, idur*.1, .01
i1      =       p7*imodfrq			;p7=modulation index start
i2      =       p8*imodfrq			;p8=modulation index end
adev    line    i1, idur, i2			;modulation frequency
aindex  line    p7, idur, p8			;modulation index

ar      linseg  1, .1, p9, p3-.2, p10, .1, 1	; r value envelope: p9-p10 =exp. partial strength parameter start and end
amp1    =       (aindex*(ar+(1/ar)))/2
afmod   oscili  amp1, imodfrq, 1		;FM modulator (sine)
atab    =       (aindex*(ar-(1/ar)))/2		;index to table
alook   tablei  atab, 37			;table lookup to GEN12
aamod   oscili  atab, adev, 2			;am modulator (cosine)
aamod   =       (exp(alook+aamod))*aenv
acar    oscili  aamod, afmod+icarfrq, 1		;AFM (carrier)
asig    balance acar, aenv
        outs    asig, asig
        
endin       

</CsInstruments>
<CsScore>
f 1     0   8192    10  1               
f 2     0   8192    9   1 1 90          
f37 0   1024    -12 40		;Bessel function-defined from 0 to 40

i 1  0  2  .2  800  800    1    6   .1   2
i 1  +  .  .   1900 147    8    1    4   .2
i 1  .  .  .   1100 380    2    9   .5   2
i 1  .  10 .   100  100    11   3   .2   5
s
i 1  0  1 .1   200 100     1    6   .1   2
i 1  +  .  <   <    <      <    <   <    <
i 1  +  .  .   <    <      <    <   <    <
i 1  +  .  .   <    <      <    <   <    <
i 1  +  .  .   <    <      <    <   <    <
i 1  +  .  .   <    <      <    <   <    <
i 1  +  .  .   <    <      <    <   <    <
i 1  +  10 .2  800  800    9    1   .9   6
s
i 1  0  11 .25 50   51     1    6   .1   2
i 1  1  9  .05 700  401    1    6   .1   2
i 1  2  8  .   900  147    8    1    4   .2
i 1  3  7  .   1100 381    2    9   .5   2
i 1  4  6  .   200  102    11   3   .2   5
i 1  5  6  .   800  803    9    1   .9   6
e
</CsScore>
</CsoundSynthesizer>

