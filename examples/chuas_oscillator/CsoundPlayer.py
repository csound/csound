import csnd

class CsoundPlayer(object):
    def __init__(self, circleMapFrame):
        self.csound = circleMapFrame.csound
    def playThreadRoutine(self):
        try:
            print 'playThreadRoutine...'
            self.csound.setOrchestra('''

sr      = 44100
ksmps   = 100
nchnls  = 2
0dbfs   = 1000
; Set up global control channels for use by Csound API.
giy0                init            1.0
giy0                chnexport       "giy0", 3
gkw                 init            1.0
gkw                 chnexport       "gkw", 3
gkk                 init            1.0
gkk                 chnexport       "gkk", 3
gkf                 init            1.0
gkf                 chnexport       "gkf", 3
gka                 init            100.0
gka                 chnexport       "gka", 3
gkcf                init            1500
gkcf                chnexport       "gkcf", 3
gkq                 init            .7
gkq                 chnexport       "gkq", 3
gkrd                init            .7
gkrd                chnexport       "gkrd", 3
gkrw                init            .7
gkrw                chnexport       "gkrw", 3
; High-precision sine wavetable.
gisine              ftgen           1, 0, 65537, 10, 1
; Circle map opcode (See Essl, daFX 2006)
                    opcode          circlemap, a, ikkk
iy0, kw, kk, kf     xin     
ay0                 init            iy0 
ay1                 =               ( ay0 + kw - ( kk / 6.28318530717 * kf ) * sin( 6.28318530717 * kf * ay0 ) ) % 1.0  
ay0                 =               ay1
                    xout            ay0
                    endop
; Instrument to control circle map
                    instr           1
                    print           p1, p2, p3
asig                circlemap       giy0, gkw, gkk, gkf
; Anti-clicking.
aenv                linsegr         0.0, 0.1, 1.0, p3, 1.0, 0.1, 0.0
; Resonant filter.
ares                moogladder      asig, gkcf, gkq, 1.00
; Compensate for ringing.
asig                balance         ares, asig
asig                =               asig * gka
; Reverberation
arleft, arright     reverbsc        asig, asig, gkrd, 14000 
arwetleft           =               gkrw * arleft
arwetright          =               gkrw * arright
ardry               =               (1.0 - gkrw) * asig
                    outs            aenv * (arwetleft + ardry), aenv * (arwetright + ardry)
                    endin
            ''')
            self.csound.setScore('''
f0 6000
i1 1 -1
            ''')
            self.csound.setCommand('csound -h -m7 -d -o%s -B400 -b400 temp.orc temp.sco' % (self.audioOutputText.GetValue()))
            self.csound.exportForPerformance()
            self.csound.compile()
            self.csoundPerformanceThread = csnd.CsoundPerformanceThread(self.csound)
            self.csoundPerformanceThread.Play()
            print 'Playing...'
        except:
            traceback.print_exc()
        
