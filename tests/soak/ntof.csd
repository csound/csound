<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>
<CsInstruments>

/*

Example for ntof: notename to frequency 

A notename is a string of the form

<octave><pitchclass>[cents]

Example notenames are:

"4A"     -> A in the 4th octave
"5C+31"  -> C5, 31 cents higher
"4Db-13" -> D flat, 4th octave, 13 cents lower
"7F#+"   -> F# quarter tone higher 
"5G-"    -> G5 a quarter tone lower

NB: the frequency returned by ntof depends on the value of A4

*/

A4 = 442

instr 1
    ifreq ntof "4A"
    print ifreq

    koctave = 1
    while (koctave <= 8) do
        Snote sprintfk "%dC", koctave
        kfreq ntof Snote
        printf "notename: %s  freq: %.2f \n", koctave, Snote, kfreq
        koctave += 1
    od

    turnoff
endin

</CsInstruments>
<CsScore>

i 1 0 1

</CsScore>
</CsoundSynthesizer>
