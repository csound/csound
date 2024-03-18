<CsoundSynthesizer>
; network_recv.csd
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
</CsOptions>
<CsInstruments>

  sr	    =  44100
  ksmps	    =  128
  nchnls    =  1

; Example by Jonathan Murphy and Andres Cabrera 2007
; Use file OSCmidisend.csd to generate OSC events for this file

  0dbfs	    =  1

  gilisten  OSCinit   47120

  gisin	    ftgen     1, 0, 16384, 10, 1
  givel	    ftgen     2, 0, 128, -2, 0
  gicc	    ftgen     3, 0, 128, -7, 100, 128, 100  ;Default all controllers to 100
 
;Define scale tuning
  giji_12   ftgen     202, 0, 32, -2, 12, 2, 256, 60, 1, 16/15, 9/8, 6/5, 5/4, 4/3, 7/5, \
                               3/2, 8/5, 5/3, 9/5, 15/8, 2

#define DEST #"/midi"#
; Use controller number 7 for volume
#define VOL #7#

turnon 1000


    instr   1000

  kst	    init      0
  kch	    init      0
  kd1	    init      0
  kd2	    init      0

next:

  kk	    OSClisten	gilisten, $DEST, "iiii", kst, kch, kd1, kd2

if (kk == 0) goto done

printks "kst = %i, kch = %i, kd1 = %i, kd2 = %i\\n", \
         0, kst, kch, kd1, kd2

if (kst == 176) then
;Store controller information in a table
	    tablew    kd2, kd1, gicc
endif  

if (kst == 144) then
;Process noteon and noteoff messages.
  kkey	    =  kd1
  kvel	    =  kd2
  kcps	    cpstun    kvel, kkey, giji_12
  kamp	    =  kvel/127

if (kvel == 0) then
	    turnoff2  1001, 4, 1
elseif (kvel > 0) then
	    event     "i", 1001, 0, -1, kcps, kamp
endif
endif

	kgoto	next  ;Process all events in queue

done:
    endin



    instr 1001   ;Simple instrument

  icps	    init      p4
  kvol	    table     $VOL, gicc  ;Read MIDI volume from controller table
  kvol	    =  kvol/127

  aenv     linsegr    0, .003, p5, 0.03, p5 * 0.5, 0.3, 0
  aosc	    oscil     aenv, icps, gisin

	    out	      aosc * kvol
    endin


</CsInstruments>
<CsScore>
f 0 3600  ;Dummy f-table
e
</CsScore>
</CsoundSynthesizer>