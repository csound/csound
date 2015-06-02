<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o biquad-2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

/*  modal synthesis using biquad filters as oscillators
    Example by Scott Lindroth 2007 */


instr 1

    ipi = 3.1415926
    idenom = sr*0.5

    ipulseSpd = p4
    icps     = p5
    ipan = p6
    iamp    = p7
    iModes = p8

    apulse    mpulse iamp, 0

    icps    = cpspch( icps )

    ; filter gain

    iamp1 = 600
    iamp2 = 1000
    iamp3 = 1000
    iamp4 = 1000
    iamp5 = 1000
    iamp6 = 1000

    ; resonance

    irpole1 = 0.99999
    irpole2 = irpole1
    irpole3 = irpole1
    irpole4 = irpole1
    irpole5 = irpole1
    irpole6 = irpole1

    ; modal frequencies

    if (iModes == 1) goto modes1
    if (iModes == 2) goto modes2
    
    modes1:
    if1    = icps * 1            ;pot lid
    if2    = icps * 6.27
    if3    = icps * 3.2
    if4    = icps * 9.92
    if5    = icps * 14.15
    if6    = icps * 6.23
    goto nextPart

    modes2:
    if1     = icps * 1            ;uniform wood bar
    if2     = icps * 2.572
    if3     = icps * 4.644
    if4     = icps * 6.984
    if5     = icps * 9.723
    if6     = icps * 12.0
    goto nextPart

    nextPart:

    ; convert frequency to radian frequency

    itheta1 = (if1/idenom) * ipi
    itheta2 = (if2/idenom) * ipi
    itheta3 = (if3/idenom) * ipi
    itheta4 = (if4/idenom) * ipi
    itheta5 = (if5/idenom) * ipi
    itheta6 = (if6/idenom) * ipi

    ; calculate coefficients

    ib11 = -2 * irpole1 * cos(itheta1)
    ib21 = irpole1 * irpole1
    ib12 = -2 * irpole2 * cos(itheta2)
    ib22 = irpole2 * irpole2
    ib13 = -2 * irpole3 * cos(itheta3)
    ib23 = irpole3 * irpole3
    ib14 = -2 * irpole4 * cos(itheta4)
    ib24 = irpole4 * irpole4
    ib15 = -2 * irpole5 * cos(itheta5)
    ib25 = irpole5 * irpole5
    ib16 = -2 * irpole6 * cos(itheta6)
    ib26 = irpole6 * irpole6

    ;printk 1, ib 11
    ;printk 1, ib 21

    ;  also try setting the -1 coeff. to 0, but be sure to scale down the amplitude!

    asin1     biquad  apulse * iamp1, 1, 0, -1, 1, ib11, ib21
         asin2       biquad  apulse * iamp2, 1, 0, -1, 1, ib12, ib22
         asin3       biquad  apulse * iamp3, 1, 0, -1, 1, ib13, ib23
         asin4       biquad  apulse * iamp4, 1, 0, -1, 1, ib14, ib24
         asin5       biquad  apulse * iamp5, 1, 0, -1, 1, ib15, ib25
         asin6       biquad  apulse * iamp6, 1, 0, -1, 1, ib16, ib26


    afin    =    (asin1 + asin2 + asin3 + asin4 + asin5 + asin6)

    outs        afin * sqrt(p6), afin*sqrt(1-p6)

endin
</CsInstruments>
<CsScore>
;ins     st    dur  pulseSpd   pch    pan    amp     Modes
i1        0    12    0       7.089    0      0.7    2
i1        .    .    .        7.09     1      .      .
i1        .    .    .        7.091    0.5    .      .

i1        0    12    0       8.039    0      0.7    2
i1        0    12    0       8.04     1      0.7    2
i1        0    12    0       8.041    0.5    0.7    2

i1        9    .    .        7.089    0      .      2
i1        .    .    .        7.09     1      .      .
i1        .    .    .        7.091    0.5    .      .

i1        9    12    0       8.019    0      0.7    2
i1        9    12    0       8.02     1      0.7    2
i1        9    12    0       8.021    0.5    0.7    2
e
</CsScore>
</CsoundSynthesizer>
