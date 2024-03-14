<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pconvolve.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; additions by Menno Knevel 2022

sr = 44100
ksmps = 32
nchnls = 2
nchnls_i = 1    ; assume only one mono signal for audio input!
0dbfs  = 1

instr   1

kmix = .5                                   ; Wet/dry mix
kvol  = .05*kmix                            ; volume level of reverb
                                
kmix = (kmix < 0 || kmix > 1 ? .5 : kmix)   ; do some safety checking
kvol  = (kvol < 0 ? 0 : .5*kvol*kmix)       ; to make sure we the parameters a good

ipartitionsize = p4                         ; size of each convolution partion
                                            ; for best performance, this parameter needs to be tweaked
idel = (ksmps < ipartitionsize ? ipartitionsize + ksmps : ipartitionsize)/sr    ; calculate latency
prints "\nConvolving with a latency of %f seconds\n", idel
prints "***if no live input is given, nothing will sound...***\n\n"

alive in                                    ; get live input (mono)
awetl, awetr pconvolve kvol* (alive), "drumsSlp.wav", ipartitionsize

adryl delay (1-kmix)*alive, idel            ; delay dry signal, to align it with the convoled sig
adryr delay (1-kmix)*alive, idel
      outs adryl+awetl, adryr+awetr

endin
    
instr 2

imix = 0.5                                  ; Wet/dry mix
ivol = .05*imix                             ; volume level of reverb when wet/dry mix is changed, to avoid clipping

ipartitionsize = 1024                       ; size of each convolution partion
idel = (ksmps < ipartitionsize ? ipartitionsize + ksmps : ipartitionsize)/sr   ; latency of pconvolve opcode

kcount  init    idel*kr
; since we are using a soundin [instead of in] we can do a kind of "look ahead"
; without output, creating zero-latency  by looping during one k-pass
loop:
    asig soundin p4, 0
    awetl, awetr pconvolve ivol*(asig),"rv_stereo.wav", ipartitionsize
    adry delay  (1-imix)*asig,idel  ; Delay dry signal, to align it with
    kcount = kcount - 1
 if kcount > 0 kgoto loop
    outs awetl+adry, awetr+adry
        
endin

</CsInstruments>
<CsScore>

i 1 0 20 1024       ;play live for 20 seconds

i 2 20 5 "fox.wav"
i 2 25 5 "flute.aiff"
e
</CsScore>
</CsoundSynthesizer>
