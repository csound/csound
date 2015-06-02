<CsoundSynthesizer>

<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o vcella.wav -W ;;; for file output any platform
</CsOptions>

<CsInstruments>
; vcella.csd
; by Anthony Kozar

; This file demonstrates some of the new opcodes available in
; Csound 5 that come from Gabriel Maldonado's CsoundAV.

sr        = 44100
kr        = 4410
ksmps     = 10
nchnls    = 1

; Cellular automata-driven oscillator bank using vcella and adsynt
instr 1
  idur      = p3
  iCArate   = p4                                ; number of times per second the CA calculates new values
	
  ; f-tables for CA parameters
  iCAinit   = p5                                ; CA initial states					
  iCArule   = p6                                ; CA rule values
  ; The rule is used as follows:
  ; the states (values) of each cell are summed with their neighboring cells within
  ; the specied radius (+/- 1 or 2 cells).  Each sum is used as an index to read a 
  ; value from the rule table which becomes the new state value for its cell.
  ; All new states are calculated first, then the new values are all applied 
  ; simultaneously.

  ielements = ftlen(iCAinit)
  inumrules = ftlen(iCArule)
  iradius   = 1

  ; create some needed tables
  iCAstate  ftgen     0, 0, ielements, -2, 0    ; will hold the current CA states
  ifreqs    ftgen     0, 0, ielements, -2, 0    ; will hold the oscillator frequency for each cell
  iamps     ftgen     0, 0, ielements, -2, 0    ; will hold the amplitude for each cell

  ; calculate cellular automata state
  ktrig     metro     iCArate                   ; trigger the CA to update iCArate times per second
            vcella    ktrig, 0, iCAstate, iCAinit, iCArule, ielements, inumrules, iradius

  ; scale CA state for use as amplitudes of the oscillator bank
            vcopy     iamps, iCAstate, ielements
            vmult     iamps, (1/3), ielements   ; divide by 3 since state values are 0-3

            vport     iamps, .01, ielements     ; need to smooth the amplitude changes for adsynt
  ; we could use adsynt2 instead of adsynt, but it does not seem to be working

  ; i-time loop for calculating frequencies
  index     =         0
  inew      =         1
  iratio    =         1.125                     ; just major second (creating a whole tone scale)
loop1:
            tableiw   inew, index, ifreqs, 0    ; 0 indicates integer indices
  inew      =         inew * iratio
  index     =         index + 1
  if (index < ielements) igoto loop1

  ; create sound with additive oscillator bank
  ifreqbase = 64
  iwavefn   = 1
  iphs      = 2                                 ; random oscillator phases

  kenv      linseg    0.0, 0.5, 1.0, idur - 1.0, 1.0, 0.5, 0.0
  aosc      adsynt    kenv, ifreqbase, iwavefn, ifreqs, iamps, ielements, iphs

            out       aosc * ampdb(68)
endin

</CsInstruments>

<CsScore>
f1 0 16384 10 1

; This example uses a 4-state cellular automata
; Possible state values are 0, 1, 2, and 3

; CA initial state
; We have 16 cells in our CA, so the initial state table is size 16
f10 0 16 -2  0 1 0 0  1 0 0 2  2 0 0 1  0 0 1 0

; CA rule
; The maximum sum with radius 1 (3 cells) is 9, so we need 10 values in the rule (0-9)
f11 0 16 -2  1 0 3 2 1  0 0 2 1 0

; Here is our one and only note!
i1 0  20  4  10  11

e

</CsScore>
</CsoundSynthesizer>