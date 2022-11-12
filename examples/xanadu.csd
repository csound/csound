<CsoundSynthesizer>
<CsOptions>
csound -m35 -R -W -f -d -o dac
</CsOptions>
<CsInstruments>
sr          =           48000
ksmps       =           128
nchnls      =           2
;--------------------------------------------------------
;Instrument 1 : plucked strings chorused left/right and
;       pitch-shifted and delayed taps thru exponential
;       functions, and delayed.
;--------------------------------------------------------

            instr       1
ishift      =           .00666667               ;shift it 8/1200.
ipch        =           cpspch(p5)              ;convert parameter 5 to cps.
ioct        =           octpch(p5)              ;convert parameter 5 to oct.
kvib        oscili      1/120, ipch/50, 1       ;vibrato
ag          pluck       2000, cpsoct(ioct+kvib), 1000, 1, 1
agleft      pluck       2000, cpsoct(ioct+ishift), 1000, 1, 1
agright     pluck       2000, cpsoct(ioct-ishift), 1000, 1, 1
af1         expon       .1, p3, 1.0             ;exponential from 0.1 to 1.0
af2         expon       1.0, p3, .1             ;exponential from 1.0 to 0.1
adump       delayr      2.0                     ;set delay line of 2.0 sec
atap1       deltapi     af1                     ;tap delay line with kf1 func.
atap2       deltapi     af2                     ;tap delay line with kf2 func.
ad1         deltap      2.0                     ;delay 2 sec.
ad2         deltap      1.1                     ;delay 1.1 sec.
            delayw      ag                      ;put ag signal into delay line.
            outs        agleft+atap1+ad1, agright+atap2+ad2
            endin
;-------------------------------------------------------------
;Instrument 2 : plucked strings chorused left/right and
;       pitch-shifted with fixed delayed taps.
;------------------------------------------------------------

            instr       2
ishift      =           .00666667               ;shift it 8/1200.
ipch        =           cpspch(p5)              ;convert parameter 5 to cps.
ioct        =           octpch(p5)              ;convert parameter 5 to oct.
kvib        oscili      1/120, ipch/50, 1       ;vibrato
ag          pluck       1000, cpsoct(ioct+kvib), 1000, 1, 1
agleft      pluck       1000, cpsoct(ioct+ishift), 1000, 1, 1
agright     pluck       1000, cpsoct(ioct-ishift), 1000, 1, 1
adump       delayr      0.3                     ;set delay line of 0.3 sec
ad1         deltap      0.1                     ;delay 100 msec.
ad2         deltap      0.2                     ;delay 200 msec.
            delayw      ag                      ;put ag sign into del line.
            outs        agleft+ad1, agright+ad2
            endin
;-----------------------------------------------------------
;Instrument 3 : New FM algorithm, modified to produce large timbre
;               shifts using modulation of I and r. Detuned chorusing employed.
;-----------------------------------------------------------
            instr       3
ishift      =           .00666667               ;shift it 8/1200.
ipch        =           cpspch(p5)              ;convert parameter 5 to cps.
ioct        =           octpch(p5)              ;convert parameter 5 to oct.
aadsr       linseg      0, p3/3, 1.0, p3/3, 1.0, p3/3, 0 ;ADSR envelope
amodi       linseg      0, p3/3, 5, p3/3, 3, p3/3, 0 ;ADSR envelope for I
amodr       linseg      p6, p3, p7              ;r moves from p6->p7 in p3 sec.
a1          =           amodi*(amodr-1/amodr)/2
a1ndx       =           abs(a1*2/20)            ;a1*2 is normalized from 0-1.
a2          =           amodi*(amodr+1/amodr)/2
a3          tablei      a1ndx, 3, 1             ;lookup tbl in f3, normal index
ao1         oscili      a1, ipch, 2             ;cosine
a4          =           exp(-0.5*a3+ao1)
ao2         oscili      a2*ipch, ipch, 2        ;cosine
aoutl       oscili      1000*aadsr*a4, ao2+cpsoct(ioct+ishift), 1 ;fnl outleft
aoutr       oscili      1000*aadsr*a4, ao2+cpsoct(ioct-ishift), 1 ;fnl outright
            outs        aoutl, aoutr
            endin

</CsInstruments>
<CsScore>
;       Score for final project in Digital Audio Processing
;       ---------------------------------------------------

;           Piece entitled :  X A N A D U (short version)
;                           Joseph T. Kung, 12/12/88

;           The first part of the score will specify all function
;       tables used in the piece. The second part specifies
;       the instruments and notes. The latter is divided into
;       7 sections, each playing a chord on a different
;                 instrument.
;       The chords are uncommon guitar chords that use the open
;       B and E strings often. These will be transposed by
;       octaves on some chords.

;       Each instrument will play a chord for 15 seconds. The
;                 timbre
;       of the instrument will change in that interval and join
;       with the next instrument/chord sequence. Instrument 3
;       uses a modified FM synthesis technique. This is joined
;       by an additional plucked-string instrument
;       (instruments 1 and 2).

;   The Function Tables
;   -------------------
;All functions are post-normalized (max value is 1) if p4 is
;POSITIVE.
f1 0 65536 10 1      ;sine wave
f2 0 65536 11 1      ;cosine wave
f3 0 65536 -12 20.0  ;unscaled ln(I(x)) from 0 to 20.0

;-----------------------------------------------------------

;----- This section comprises all the new FM sounds -----------

;F#7addB chord on a guitar
i3 0 15 0 7.06 2.0 0.2  ;F#
i3 . . . 8.01 . .   ;C# above
i3 . . . 8.06 . .   ;F# octave above 1st one
i3 . . . 8.10 . .   ;Bb next one up
i3 . . . 8.11 . .   ;B
i3 . . . 9.04 . .   ;E

;D6add9 chord on a guitar
i3 7.5 15 0 6.02 1.7 0.5    ;D
i3 . . . 6.09 . .   ;A above
i3 . . . 7.02 . .   ;D octave above 1st one
i3 . . . 7.06 . .   ;F# next one up
i3 . . . 6.11 . .   ;B
i3 . . . 7.04 . .   ;E

;Bmajadd11 chord on a guitar
i3 15 15 0 7.11 1.4 0.8 ;B
i3 . . . 8.06 . .   ;F# above
i3 . . . 8.11 . .   ;B octave above 1st one
i3 . . . 9.03 . .   ;D# next one up
i3 . . . 8.11 . .   ;B
i3 . . . 9.04 . .   ;E;

;Amajadd9 chord on a guitar
i3 22.5 15 0 6.09 1.1 1.1   ;A
i3 . . . 7.04 . .   ;E above
i3 . . . 8.09 . .   ;A octave above 1st one
i3 . . . 8.01 . .   ;C# next one up
i3 . . . 7.11 . .   ;B
i3 . . . 8.04 . .   ;E

;Bmajadd11 chord on a guitar
i3 30 15 0 6.11 0.8 1.4 ;B
i3 . . . 7.06 . .   ;F# above
i3 . . . 7.11 . .   ;B octave above 1st one
i3 . . . 8.03 . .   ;D# next one up
i3 . . . 7.11 . .   ;B
i3 . . . 8.04 . .   ;E;

;Gmaj6 chord on a guitar
i3 37.5 15 0 5.07 0.5 1.7   ;G
i3 . . . 6.02 . .   ;D above
i3 . . . 6.07 . .   ;G octave above 1st one
i3 . . . 6.11 . .   ;B on G string
i3 . . . 6.11 . .   ;B
i3 . . . 7.04 . .   ;E

;F#7addB chord on a guitar
i3 45 15 0 7.06 0.2 2.0 ;F#
i3 . . . 8.01 . .   ;C# above
i3 . . . 8.06 . .   ;F# octave above 1st one
i3 . . . 8.10 . .   ;Bb next one up
i3 . . . 8.11 . .   ;B
i3 . . . 9.04 . .   ;E

; This section adds the plucked chords to the beginning of each
; section.

;F#7addB chord on a guitar
i1 0 10 0 8.06  ;F#
i1 0.1 . . 9.01 ;C# above
i1 0.2 . . 9.06 ;F# octave above 1st one
i1 0.3 . . 9.10 ;Bb next one up
i1 0.4 . . 9.11 ;B
i1 0.5 . . 10.04    ;E

;D6add9 chord on a guitar
i2 7.5 10 0 8.02    ;D
i2 7.6 . . 8.09     ;A above
i2 7.7 . . 9.02     ;D octave above 1st one
i2 7.8 . . 9.06     ;F# next one up
i2 7.9 . . 9.11     ;B
i2 8.0 . . 10.04    ;E

;Bmajadd11 chord on a guitar
i2 15 10 0 8.11     ;B
i2 15.1 . . 9.06    ;F# above
i2 15.2 . . 9.11    ;B octave above 1st one
i2 15.3 . . 10.03   ;D# next one up
i2 15.4 . . 9.11    ;B
i2 15.5 . . 10.04   ;E;

;Amajadd9 chord on a guitar
i2 22.5 10 0 8.09   ;A
i2 22.6 . . 9.04    ;E above
i2 22.7 . . 10.09   ;A octave above 1st one
i2 22.8 . . 10.01   ;C# next one up
i2 22.9 . . 9.11    ;B
i2 23.0 . . 10.04   ;E

;Bmajadd11 chord on a guitar
i2 30 10 0 8.11     ;B
i2 30.1 . . 9.06    ;F# above
i2 30.2 . . 9.11    ;B octave above 1st one
i2 30.3 . . 10.03   ;D# next one up
i2 30.4 . . 9.11    ;B
i2 30.5 . . 10.04   ;E;

;Gmaj6 chord on a guitar
i2 37.5 10 0 8.07   ;G
i2 37.6 . . 9.02    ;D above
i2 37.7 . . 9.07    ;G octave above 1st one
i2 37.8 . . 9.11    ;B on G string
i2 37.9 . . 9.11    ;B
i2 38.0 . . 10.04   ;E

;F#7addB chord on a guitar
i1 45 10 0 9.06     ;F#
i1 45.1 . . 10.01   ;C# above
i1 45.2 . . 10.06   ;F# octave above 1st one
i1 45.3 . . 10.10   ;Bb next one up
i1 45.4 . . 10.11   ;B
i1 45.5 . . 11.04   ;E
e


</CsScore>
</CsoundSynthesizer>
