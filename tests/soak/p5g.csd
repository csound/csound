<CsoundSynthesizer>
<CsOptions>
-+rtaudio=alsa -o dac:hw:0
</CsOptions>
<CsInstruments>
nchnls = 1
ksmps = 1000

#define P5G_BUTTONS     #0#
#define P5G_BUTTON_A    #1#
#define P5G_BUTTON_B    #2#
#define P5G_BUTTON_C    #4#
#define P5G_JUSTPUSH    #8#
#define P5G_JUSTPU_A    #9#
#define P5G_JUSTPU_B    #10#
#define P5G_JUSTPU_C    #12#
#define P5G_RELEASED    #16#
#define P5G_RELSED_A    #17#
#define P5G_RELSED_B    #18#
#define P5G_RELSED_C    #20#
#define P5G_FINGER_INDEX #32#
#define P5G_FINGER_MIDDLE #33#
#define P5G_FINGER_RING	 #34#
#define P5G_FINGER_PINKY #35#
#define P5G_FINGER_THUMB #36#
#define P5G_DELTA_X     #37#
#define P5G_DELTA_Y     #38#
#define P5G_DELTA_Z     #39#
#define P5G_DELTA_XR    #40#
#define P5G_DELTA_YR    #41#
#define P5G_DELTA_ZR    #42#
#define P5G_ANGLES      #43#

gka   init 0
gkp   init 0

instr 1  
      p5gconnect
  ka  p5gdata    $P5G_JUSTPU_A.
  kc  p5gdata    $P5G_BUTTON_C.
; If the A button is just pressed then activate a note
  if  (ka==0)    goto ee
  event          "i", 2, 0, 2

ee:
  gka p5gdata    $P5G_DELTA_X.
  gkp p5gdata    $P5G_DELTA_Y.
  printk2 gka
  printk2 gkp
  if  (kc==0)    goto ff
  printks "turning off (%d)\n", 0, kc 
  turnoff
ff:
endin

instr 2
  a1 oscil  ampdbfs(gkp), 440+100*gka, 1
;;  a1 oscil  10000, 440, 1
     out   a1
endin

</CsInstruments>

<CsScore>
f1 0 4096 10 1
i1 0 300

</CsScore>

</CsoundSynthesizer>
