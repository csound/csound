<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n     ;;;no sound
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o rtclock.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

;after an example by Iain McCurdy

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

FLcolor	200, 200, 200, 0, 0, 0
;	LABEL     | WIDTH | HEIGHT | X | Y
FLpanel	"rtclock",   500,    130,    0,  0
;                                  ON,OFF,TYPE,WIDTH, HEIGHT, X, Y, OPCODE, INS,START,IDUR
gkOnOff,ihOnOff	FLbutton "On/Off", 1,  0,  22,  150,   25,    5, 5,    0,   1,   0,   3600
gkExit,ihExit	FLbutton "exitnow",1,  0,  21,  150,   25,  345, 5,    0,  999,  0,   0.001
FLsetColor2 255, 0, 50, ihOnOff	;reddish color

;VALUE DISPLAY BOXES	 WIDTH,HEIGHT,X, Y
gidclock FLvalue "clock", 100, 25, 200, 60
FLsetVal_i 1, ihOnOff	
FLpanel_end
FLrun

instr 1	

if gkOnOff !=0 kgoto CONTINUE ;sense if FLTK on/off switch is not off (in which case skip the next line)  
turnoff			      ;turn this instr. off now
CONTINUE:
ktime rtclock                 ;clock continues to run even 
FLprintk2 ktime, gidclock     ;after the on/off button was used to stop

endin

instr 999

exitnow			      ;exit Csound as fast as possible

endin
</CsInstruments>
<CsScore>

f 0 60	;runs 60 seconds
e
</CsScore>
</CsoundSynthesizer>
