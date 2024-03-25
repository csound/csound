<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o seqtime.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>

 sr = 44100
 ksmps = 64
 nchnls = 1

; By Tim Mortimer and Andres Cabrera 2007

0dbfs = 1

gisine         ftgen    0, 0, 8192, 10,    1
;;; table defining an integer pitch set
gipset    ftgen     0, 0, 4, -2, 8.00, 8.04, 8.07, 8.10
;;;DELTA times for seqtime
gidelta    ftgen    0, 0, 4, -2, .5, 1, .25, 1.25


  instr 1

kndx init 0
ktrigger init 0

ktime_unit init 1
kstart init p4
kloop init p5
kinitndx init 0
kfn_times init gidelta

ktrigger seqtime ktime_unit, kstart, kloop, kinitndx, kfn_times

printk2 ktrigger


if (ktrigger > 0) then
   kpitch table kndx, gipset
   event "i", 2, 0, 1, kpitch
   kndx = kndx + 1
   kndx = kndx % kloop
endif

  endin


  instr 2
icps = cpspch (p4)
a1    buzz    1, icps, 7, gisine
aamp expseg    0.00003,.02,1,p3-.02,0.00003

a1 = a1 * aamp * 0.5

out a1
  endin

</CsInstruments>

<CsScore>
;      start    dur   kstart  kloop
i 1	0	7	0	4
i 1	8	10	0	3
i 1	19	10	4	4

</CsScore>

</CsoundSynthesizer>
