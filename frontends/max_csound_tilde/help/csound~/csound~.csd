<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 16
nchnls  = 4

; instr 1 is an always-on score activated instrument that 
; exists mainly to get "control" message values from [csound~].
;
; instr 2 is an FM MIDI instrument.
;
; instr 3 is a score activated FM instrument.
;
; instr 4 is an always-on score activated Effects instrument.
; It processes audio from instr 2, 3 with a reverb and flanger
; effect (respectively).

massign 0, 0 ; Disable default MIDI assignments.
massign 1, 2 ; Assign MIDI channel 1 to instr 2.
zakinit 2, 1 ; Create 2 a-rate zak channels and 1 k-rate zak channel.
giSine ftgen 1, 0, 16384, 10, 1 ; Generate a sine wave table.

; Initialize MIDI control values. 
initc7 1, 1, 0
initc7 1, 2, .5
initc7 1, 3, .25
initc7 1, 4, .2

; Declare all "chn" channels.  Always declare channels in the header
; section.  Declaring is required for output channels.  If an output
; channel is not declared using chn_k, [csound~] will not output value
; pairs for that channel.
chn_k "car", 3      ; 3 = input + output
chn_k "mod", 3
chn_k "ndx", 3
chn_k "portTime", 1 ; 1 = input
chn_k "carX2", 2    ; 2 = output

; chnget instrument.  Always-on and score activated.
instr 1
    ; Get the values for "car", "mod", "ndx", and "portTime". 
    ; We are getting some of these values at i-rate as well as
    ; k-rate.  The i-rate values are needed for the portk opcodes.
    iCar  chnget "car"
    iMod  chnget "mod"
    iNdx  chnget "ndx" 
    gkCar chnget "car"
    gkMod chnget "mod"
    gkNdx chnget "ndx"
    kTim  chnget "portTime"

    ; We could put the chnget/chnset opcodes in the MIDI activated
    ; instrument (2), but then we would have redundant calls because
    ; it's polyphonic.

    ; Smooth out jumps.
    gkCar portk gkCar, kTim, iCar
    gkMod portk gkMod, kTim, iMod
    gkNdx portk gkNdx, kTim, iNdx

    chnset gkCar * 2, "carX2" ; Just demonstrates the chnset opcode.
endin

; Polyphonic FM instrument.  MIDI activated.
instr 2
    iCps cpsmidi
    iAmp veloc 0, 32768
    
    ; Generate the FM signal.
    aout foscil iAmp, iCps, gkCar, gkMod, gkNdx, giSine
    
    ; Read in MIDI cc's as normalized [0,1] values.
    iAtk midic7 1, 0, 1
    iDec midic7 2, 0, 1
    iSus midic7 3, 0, 1
    iRel midic7 4, 0, 1

    ; Make the MIDI knobs behave exponentially by squaring,
    ; then multiply by max length in seconds.
    iAtk = iAtk * iAtk * 3 + .001
    iDec = iDec * iDec * 3 + .001
    iSus = iSus * iSus + .001
    iRel = iRel * iRel * 4 + .001

    ; Generate and apply the envelope.
    aenv expsegr 0.001, iAtk, 1, iDec, iSus, iRel, 0.001
    aout = aout * aenv

    ; Mix into zak channel 1.
    zawm aout, 1
endin
    
; FM instrument.  Score activated.
instr 3
    kCar line p6, p3, p7
    kMod line p8, p3, p9
    kNdx line p10, p3, p11
    amp  linen .85, p3 * .5, p3, p3 * .5
    aout foscil p5, cpspch(p4), kCar, kMod, kNdx, giSine
    aout = aout * amp * .25

    ; Mix into zak channel 2.
    zawm aout, 2
endin

; Effects instrument.  Always-on and score activated.
instr 4
    a2 zar 1      ; Read instr 2 audio.
    a3 zar 2      ; Read instr 3 audio.
    denorm a2, a3 ; Prevent CPU spikes on Intel processors.

    a2R, a2R freeverb a2, a2, .7, .7 ; Apply reverb effect.
    a2R = a2R * .45 + a2 * .55       ; Mix dry and wet signals.
    outch 1, a2R, 2, a2R

    aDel oscili .5, .69, giSine      ; LFO for flanger delay time
    aDel = (aDel + .5) * .04 + .06   ; Condition LFO signal.
    a3F flanger a3, aDel, .6         ; Apply flanger effect.
    a3F = a3F * .45 + a3 * .55       ; Mix dry and wet signals.
    outch 3, a3F, 4, a3F

    zacl 0, 2  ; Clear audio channels to prevent audio build-up.
endin

</CsInstruments>
<CsScore>

f0 3600
i1 0 3600 ; Activate the always-on chnget instrument.
i4 0 3600 ; Activate the Reverb/Flanger always-on instrument.
e

</CsScore>
</CsoundSynthesizer>





