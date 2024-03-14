<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tablei.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

seed 0	;generate new values every time the instr is played

instr 1

ifn = p4
isize = p5
ithresh = 0.5
    
itemp ftgen ifn, 0, isize, 21, 2

iwrite_value = 0
i_index = 0
    
loop_start:
    iread_value tablei i_index, ifn
    
    if iread_value > ithresh then
         iwrite_value = 1
    else
         iwrite_value = -1
    endif
tableiw iwrite_value, i_index, ifn
loop_lt i_index, 1, isize, loop_start        
    turnoff

endin

instr 2

ifn = p4
isize = ftlen(ifn)    
prints "Index\tValue\n"
    
i_index = 0
loop_start:
    ivalue tablei i_index, ifn
    prints "%d:\t%f\n", i_index, ivalue

  loop_lt i_index, 1, isize, loop_start		;read table 1 with our index

aout oscili .5, 100, ifn			;use table to play the polypulse
     outs   aout, aout

endin
</CsInstruments>
<CsScore>
i 1 0 1 100 16
i 2 0 2 100
e
</CsScore>
</CsoundSynthesizer>
