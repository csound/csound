<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o diskgrain.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

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

a1 diskgrain "mary.wav", 1, ifreq, ipitch, igrsize, ips*istr, 1, iolaps
   outs   a1, a1

endin

</CsInstruments>
<CsScore>
f 1 0 8192 20 2 1  ;Hanning function

;           timescale   pitchscale
i 1   0   5     1           1
i 1   +   5     2           1
i 1   +   5     1          0.75
i 1   +   5     1.5        1.5
i 1   +   5     0.5        1.5

e
</CsScore>
</CsoundSynthesizer>