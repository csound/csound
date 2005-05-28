<CsoundSynthesizer>
<CsOptions>
csound -A -d -o trymurph.aiff temp.orc temp.sco
</CsOptions>
<CsInstruments>
; originally tryit.orc
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

;
; Morph the partials in clarinet.sdif into the
; partials in flute.sdif over the duration of
; the sustained portion of the two tones (from
; .2 to 2.0 seconds in the clarinet, and from
; .5 to 2.1 seconds in the flute). The onset
; and decay portions in the morphed sound are
; specified by parameters p4 and p5, respectively.
; The morphing time is the time between the
; onset and the decay. The clarinet partials are
; shfited in pitch to match the pitch of the flute
; tone (D above middle C).
;
instr 1
    ionset   =           p4
    idecay   =           p5
    itmorph  =           p3 - (ionset + idecay)
    ipshift  =           cpspch(8.02)/cpspch(8.08)

    ktcl     linseg      0, ionset, .2, itmorph, 2.0, idecay, 2.1    ; clarinet time function, morph from .2 to 2.0 seconds
    ktfl     linseg      0, ionset, .5, itmorph, 2.1, idecay, 2.3    ; flute time function, morph from .5 to 2.1 seconds
    kmurph   linseg      0, ionset, 0, itmorph, 1, idecay, 1
             lorisread   ktcl, "clarinet.sdif", 1, ipshift, 2, 1, .001
             lorisread   ktfl, "flute.sdif", 2, 1, 1, 1, .001
             lorismorph  1, 2, 3, kmurph, kmurph, kmurph
    asig     lorisplay   3, 1, 1, 1
             out         asig
endin

;
; Morph the partials in trombone.sdif into the
; partials in meow.sdif. The start and end times
; for the morph are specified by parameters p4
; and p5, respectively. The morph occurs over the
; second of four pitches in each of the sounds,
; from .75 to 1.2 seconds in the flutter-tongued
; trombone tone, and from 1.7 to 2.2 seconds in
; the cat's meow. Different morphing functions are
; used for the frequency and amplitude envelopes,
; so that the partial amplitudes make a faster
; transition from trombone to cat than the frequencies.
; (The bandwidth envelopes use the same morphing
; function as the amplitudes.)
;
instr 2
    ionset   =           p4
    imorph   =           p5 - p4
    irelease =           p3 - p5

    kttbn    linseg      0, ionset, .75, imorph, 1.2, irelease, 2.4
    ktmeow   linseg      0, ionset, 1.7, imorph, 2.2, irelease, 3.4

    kmfreq   linseg      0, ionset, 0, .75*imorph, .25, .25*imorph, 1, irelease, 1
    kmamp    linseg      0, ionset, 0, .75*imorph, .9, .25*imorph, 1, irelease, 1

             lorisread   kttbn, "trombone.sdif", 1, 1, 1, 1, .001
             lorisread   ktmeow, "meow.sdif", 2, 1, 1, 1, .001
             lorismorph  1, 2, 3, kmfreq, kmamp, kmamp
    asig     lorisplay   3, 1, 1, 1
             out         asig
endin

;;
;; Morph the partials in carhorn.sdif into
;; the partials in meow.sdif linearly over
;; all but the last 2 seconds of the note.
;; The morph is performed over the first
;; .75 seconds of the source sounds. The last
;; 2.5 seconds (of meow) is unmodified.
;; Use 1 ms fade time.
;;
;instr 2
;    ktime1   linseg      0, p3, 3.4
;    ktime2   linseg      0, p3, 1.25
;    kmurph   linseg      0, p3/3, 0, p3/3, 1, p3/3, 1
;
;             lorisread   ktime1, "meow3.sdif", 1, 1, 1, 1, .001
;             lorisread   ktime2, "carhorn.sdif", 2, 1, 1, 1, .001
;             lorismorph  1, 2, 3, kmurph, kmurph, kmurph
;       asig     lorisplay   3, 1, 1, 1
;             out         asig
;endin

</CsInstruments>
<CsScore>
; play instr 1
;     strt   dur   onset   decay
i 1    0      3     .25     .15
i 1    +      1     .10     .10
i 1    +      6    1.      1.

f 0    11
s

; play instr 2
;     strt   dur  morph_start   morph_end
i 2    0      4     .75           2.75

e

</CsScore>
</CsoundSynthesizer>
