<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o dates.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

seed     0 ;each time different seed

instr 1
;;generating a different filename each time csound renders
itim     date
Stim     dates     itim
Syear    strsub    Stim, 20, 24
Smonth   strsub    Stim, 4, 7
Sday     strsub    Stim, 8, 10
iday     strtod    Sday
Shor     strsub    Stim, 11, 13
Smin     strsub    Stim, 14, 16
Ssec     strsub    Stim, 17, 19
Sfilnam  sprintf  "%s_%s_%02d_%s_%s_%s.wav", Syear, Smonth, iday, Shor,Smin, Ssec
;;rendering with random frequency, amp and pan, and writing to disk
ifreq    random    400, 1000
iamp     random    .1, 1
ipan     random    0, 1
asin     oscils    iamp, ifreq, 0
aL, aR   pan2      asin, ipan
         fout      Sfilnam, 14, aL, aR
         outs      aL, aR
         printf_i  "File '%s' written to the same directory as this CSD file is!\n", 1, Sfilnam

endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>