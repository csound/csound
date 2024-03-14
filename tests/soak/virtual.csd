<CsoundSynthesizer>
<CsOptions>; Select audio/midi flags here according to platform
; Audio out   Audio in     Virtual MIDI    -M0 is needed anyway
-odac           -iadc    -+rtmidi=virtual -M0
</CsOptions>

<CsInstruments>
; By Mark Jamerson 2007

sr=44100
ksmps=10
nchnls=2

massign 1,1
prealloc 1,10

instr 1  ;Midi FM synth 

inote cpsmidi
iveloc ampmidi 10000
idur = 2
    xtratim 1

kgate oscil 1,10,2
anoise noise 100*inote,.99
acps  samphold anoise,kgate
aosc oscili 1000,acps,1
aout = aosc

; Use controller 7 to control volume
kvol ctrl7 1, 7, 0.2, 1

outs kvol * aout, kvol * aout

endin

</CsInstruments>

<CsScore>
f0 3600
f1 0 1024 10 1 
f2 0 16 7 1 8 0 8
f3 0 1024 10 1 .5 .6 .3 .2 .5

e 
</CsScore>
</CsoundSynthesizer>