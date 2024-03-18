<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   No messages  MIDI in
-odac            -d         -M0  ;;;RT audio I/O with MIDI in
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o chanctrl.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; press your midi keyboard and move your midi controller to see result

ichnl  = 1		;MIDI note inputs on channel 1
ictlno = 7		;use midi volume controller 
kch  chanctrl ichnl, 7	;to control amplitude of oscil
     printk2  kch

asig oscil kch*(1/127), 220, 1
     outs  asig, asig
endin

</CsInstruments>
<CsScore>
;Dummy f-table to give time for real-time MIDI events
f 0 30
;sine wave.
f 1 0 16384 10 1
e

</CsScore>
</CsoundSynthesizer>
