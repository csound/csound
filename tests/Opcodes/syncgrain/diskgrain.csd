<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o diskgrain.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; additions by Menno Knevel 2022

sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

instr 1

iolaps  = 2
igrsize = 0.04
ifreq   = iolaps/igrsize
ips     = 1/iolaps

istr = p4  /* timescale */
ipitch = p5 /* pitchscale */

a1 diskgrain "marimba.aif", 1, ifreq, ipitch, igrsize, ips*istr, 1, iolaps
prints "\nmarimba is playing....\n\n"
   outs   a1, a1

endin

instr 2

iolaps  = 5
igrsize = 0.1
ifreq   = iolaps/igrsize
ips     = 1/iolaps

istr = p4  /* timescale */
ipitch = p5 /* pitchscale */

a1 diskgrain "singFemale.aif", .5, ifreq, ipitch, igrsize, ips*istr, 1, iolaps
prints "\nFemale is singing....\n\n"
   outs   a1, a1

endin

</CsInstruments>
<CsScore>
f 1 0 8192 20 2 1  ;Hanning function

;           timescale   pitchscale
i 1   0   6     1           1           
i 1   +   6     2           1
i 1   +   6     1          0.75
i 1   +   6     1.5        1.5
i 1   +   6.25  0.5        1.5

;           timescale   pitchscale
i 2   31  4     1           1           
i 2   +   4     2           1
i 2   +   4     1          0.75
i 2   +   5     1.5        1.5
i 2   +   7.9   0.5        1.5
e
</CsScore>
</CsoundSynthesizer>
