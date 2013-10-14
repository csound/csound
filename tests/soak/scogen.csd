<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
; -odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
-n
;Don't write audio ouput to disk
</CsOptions>
<CsInstruments>
;===========================================================
;        scogen.csd       by: Matt Ingalls
;
;    a "port" of sorts
;      of the old "mills" score generator (scogen)
;
;    this instrument creates a schottstaedt.sco file
;    to be used with the schottstaedt.orc file
;
;    as long as you dont save schottstaedt.orc as a .csd
;    file, you should be able to keep it open in MacCsound
;    and render each newly generated .sco file.
;
;===========================================================


gScoName = "/Users/matt/Desktop/schottstaedt.sco"     ; the name of the file to be generated

    sr    =    100     ; this defines our temporal resolution,
                ; an sr of 100 means we will generate p2 and p3 values
                ; to the nearest 1/100th of a second

    ksmps =    1     ; set kr=sr so we can do everything at k-rate


; some print opcodes
opcode PrintInteger, 0, k
    kval    xin
        fprintks    gScoName, "%d", kval
endop

opcode PrintFloat, 0, k
    kval    xin
        fprintks    gScoName, "%f", kval
endop

opcode PrintTab, 0, 0
    fprintks    gScoName, "%n"
endop

opcode PrintReturn, 0, 0
    fprintks    gScoName, "%r"
endop


; recursively calling opcode to handle all the optional parameters
opcode ProcessAdditionalPfields, 0, ikio
    iPtable, kndx, iNumPfields, iPfield xin

    ; additional pfields start at 5, we use a default 0 to identify the first call
    iPfield = (iPfield == 0 ? 5 : iPfield)

    if (iPfield > iNumPfields) goto endloop
        ; find our tables
        iMinTable table    2*iPfield-1, iPtable
        iMaxTable table    2*iPfield, iPtable

        ; get values from our tables
        kMin tablei    kndx, iMinTable
        kMax tablei    kndx, iMaxTable

        ; find a random value in the range and write it to the score
        fprintks gScoName, "%t%f", kMin + rnd(kMax-kMin)

        ; recursively call for any additional pfields.
        ProcessAdditionalPfields iPtable, kndx, iNumPfields, iPfield + 1
    endloop:

endop


/* ===========================================================
    Generate a gesture of i-statements

    p2 = start of the gesture
    p3 = duration of the gesture
    p4 = number of a function that contains a list of all
        function table numbers used to define the
        pfield random distribution
    p5 = scale generated p4 values according to density (0=off, 1=on) [todo]
    p6 = let durations overlap gesture duration (0=off, 1=on) [todo]
    p7 = seed for random number generator seed [todo]
  ===========================================================
*/
instr Gesture

    ; initialize
    iResolution = 1/sr

    kNextStart init p2
    kCurrentTime init p2

    iNumPfields table        0, p4
    iInstrMinTable table    1, p4
    iInstrMaxTable table    2, p4
    iDensityMinTable table    3, p4
    iDensityMaxTable table    4, p4
    iDurMinTable table    5, p4
    iDurMaxTable table    6, p4
    iAmpMinTable table    7, p4
    iAmpMaxTable table    8, p4

    ; check to make sure there is enough data
    print iNumPfields
    if iNumPfields < 4 then
        prints "%dError: At least 4 p-fields (8 functions) need to be specified.%n", iNumPfields
        turnoff
    endif

    ; initial comment
    fprints    gScoName, "%!Generated Gesture from %f to %f seconds%n %!%t%twith a p-max of %d%n%n", p2, p3, iNumPfields

    ; k-rate stuff
    if (kCurrentTime >= kNextStart) then ; write a new note!

        kndx = (kCurrentTime-p2)/p3

        ; get the required pfield ranges
        kInstMin tablei    kndx, iInstrMinTable
        kInstMax tablei    kndx, iInstrMaxTable
        kDensMin tablei    kndx, iDensityMinTable
        kDensMax tablei    kndx, iDensityMaxTable
        kDurMin tablei    kndx, iDurMinTable
        kDurMax tablei    kndx, iDurMaxTable
        kAmpMin tablei    kndx, iAmpMinTable
        kAmpMax tablei    kndx, iAmpMaxTable

        ; find random values for all our required parametrs and print the i-statement
        fprintks gScoName, "i%d%t%f%t%f%t%f", kInstMin + rnd(kInstMax-kInstMin), kNextStart, kDurMin + rnd(kDurMax-kDurMin), kAmpMin + rnd(kAmpMax-kAmpMin)

        ; now any additional pfields
        ProcessAdditionalPfields p4, kndx, iNumPfields

        PrintReturn

        ; calculate next starttime
        kDensity = kDensMin + rnd(kDensMax-kDensMin)
        if (kDensity < iResolution) then
            kDensity = iResolution
        endif
        kNextStart = kNextStart + kDensity
    endif

    kCurrentTime = kCurrentTime + iResolution
endin


</CsInstruments>
<CsScore>
/*
===========================================================
  scogen.sco

    this csound module generates a score file
    you specify a gesture of notes by giving
    the "gesture" instrument a number to a
    (negative) gen2 table.

    this table stores numbers to pairs of functions.
    each function-pair represents a range (min-max)
    of randomness for every pfield for the notes to
    be generated.
===========================================================
*/


; common tables for pfield ranges
f100    0    2    -7    0 2 0    ; static 0
f101    0    2    -7    1 2 1    ; static 1
f102    0    2    -7    0 2 1 ; ramp 0->1
f103    0    2    -7    1 2 0 ; ramp 1->0
f105    0    2    -7    10 2 10 ; static 10
f106    0    2    -7    .1 2 .1 ; static .1

; specific pfield ranges
f10    0    2    -7       .8 2 .01 ; density
f11    0    2    -7       8 2 4 ; pitchmin
f12    0    2    -7       8 2 12 ; pitchmax


;=== table containing the function numbers used for all the p-field distributions
;
;    p1 -     table number
;    p2 -     time table is instantiated
;    p3 -     size of table (must be >= p5!)
;    p4 -     gen# (should be = -2)
;    p5 -     number of pfields of each note to be generated
;    p6 -     table number of the function representing the minimum possible note number (p1) of a generated note
;    p7 -     table number of the function representing the maximum possible note number (p1) of a generated note
;    p8 -     table number of the function representing the minimum possible noteon-to-noteon time (p2 density) of a generated note
;    p9 -     table number of the function representing the maximum possible noteon-to-noteon time (p2 density) of a generated note
;    p10 -    table number of the function representing the minimum possible duration (p3) of a generated note
;    p11 -    table number of the function representing the maximum possible duration (p3) of a generated note
;    p12 -    table number of the function representing the maximum possible amplitude (p4) of a generated note
;    p13 -    table number of the function representing the maximum possible amplitude (p5) of a generated note
;    p14,p16.. -    table number of the function representing the minimum possible value for additional pfields (p5,p6..) of a generated note
;    p15,p17.. -    table number of the function representing the maximum possible value for additional pfields (p5,p6..) of a generated note

;        siz    2    #pds p1min    p1max p2min    p2max p3min    p3max p4min    p4max p5min    p5max p6min    p6max
f1    0    32    -2    6    101    101    10    10 101    105    100    106    11    12    100    101


;gesture definitions
;        start    dur    pTble    scale    overlap seed
i"Gesture"     0    60    1 ;todo-->0    0    123
</CsScore>

</CsoundSynthesizer>

