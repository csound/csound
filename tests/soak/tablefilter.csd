<CsoundSynthesizer>
<CsOptions>

</CsOptions>
<CsInstruments>

sr=44100
ksmps=10
nchnls=1

      gifarn init 8     ; initialise integer for Farey Sequence F_8
      gires fareyleni gifarn  ; calculate length of F_8, returns 23
      ; the table length won't be a power of 2
      ; (The length of a Farey Sequence with n > 1 is always odd)
      gilen init gires * -1

      gifarey ftgen 200, 0, gilen, "farey", gifarn, 0

      ; initialize destiniation table with 0s
      gifiltered ftgen 0, 0, gilen, 21, 1, 0

      ; initialize second destiniation table with 0s
      gifiltered2 ftgen 0, 0, gilen, 21, 1, 0

      ; table filtering opcode:   dest.         source,         mode,   threshold
      ginumpassed tablefilteri    gifiltered,   gifarey,        1,      6
      ; the threshold parameter indicates that denominators whose weights are heavier
      ; than 6 are not passing through the filter. The weight is calculated using
      ; Clarence Barlow's function of indigestibility of a number. According to this function,
      ; higher prime numbers contribute to an increased weight of any natural integer they divide.
      ; ginumpassed is the number of elements from the source table 'gifarey'
      ; that have passed the test and which have been copied to the destination table 'gifiltered'

      ; apply a different filter:
      ginumpassed2 tablefilteri   gifiltered2,  gifarey,        2,      5
      ; In mode=2 we again test the digestibility of the denominators of the
      ; fractions in the source table.
      ; The difference to mode=1 is that we now let pass only vaules from the
      ; source that are as heavy as the threshold or greater.


instr 4
      kndx init 0 ; read out elements of now filtered F_8 sequentially and print to file
      if (kndx < ginumpassed) then
         kelem tab kndx, gifiltered
         fprintks "fareyfilter_lp.txt", "%2.6f\\n", kelem
         kndx = kndx+1
      endif
endin

instr 5
      kndx init 0 ; read out elements and print to file
      if (kndx < ginumpassed2) then
         kelem tab kndx, gifiltered2
         fprintks "fareyfilter_hp.txt", "%2.6f\\n", kelem
         kndx = kndx+1
      endif
endin

</CsInstruments>
<CsScore>

i4      0     1
i5      0     1
e
</CsScore>
</CsoundSynthesizer>
