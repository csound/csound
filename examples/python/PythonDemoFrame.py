#Boa:Frame:PythonDemoFrame

import copy
import csnd6
import os
import pickle
import shutil
import threading
import traceback
import sys

'''
Psyco (http://psyco.sourceforge.net) is a just-in-time compiler for Python.
One imagines that using Psyco here will make execution 
faster and therefore smoother.
'''
try:
    import psyco
    print 'Imported pscyo.'
except:
    print 'Failed to import psyco.'

print
print 'CREATING FILENAMES FOR THIS PIECE...'
print
scriptFilename = sys.argv[0]
print 'Full Python script: %s' % scriptFilename
title = os.path.basename(scriptFilename)
print 'Base Python script: %s' % title
orcFilename = scriptFilename + '.orc'
print 'Csound orchestra:   %s' % orcFilename
scoFilename = scriptFilename + '.sco'
print 'Csound score:       %s' % scoFilename
midiFilename = scriptFilename + '.mid'
print 'MIDI filename:      %s' % midiFilename
soundfileName = scriptFilename + '.wav'
print 'Soundfile name:     %s' % soundfileName
soundfilePlayer = r'C:\WINDOWS\system32\sndrec32.exe'
print 'Soundfile player:   %s' % soundfilePlayer
author = 'Michael_Gogins'
print 'Author:             %s' % author
snapshotsFilename = scriptFilename + '.snap'
print 'Snapshots filename: %s' % snapshotsFilename

print

class Configuration(object):
    def __init__(self):
        self.orchestra = '''\
sr                      =           44100
ksmps                   =           100
nchnls                  =           2
0dbfs                   =           60000

; Bind named control channels to global variables.

gkChebyshevpoly1 	    chnexport   "gkChebyshevpoly1", 3
gkChebyshevpoly1        init        1.0
gkChebyshevpoly2        chnexport   "gkChebyshevpoly2", 3
gkChebyshevpoly2        init        1.0
gkChebyshevpoly3        chnexport   "gkChebyshevpoly3", 3
gkChebyshevpoly3        init        1.0
gkReverbscFeedback      chnexport   "gkReverbscFeedback", 3
gkReverbscFeedback      init        0.9
gkMasterLevel           chnexport   "gkMasterLevel", 3
gkMasterLevel           init        1.0

gareverb1               init        0
gareverb2               init        0

                        instr 1
iattack                 init        p3 / 3.0
idecay                  init        p3 / 3.0
isustain 		init 	    p3 - iattack
p3 			=           p3 + idecay
			print       p1, p2, p3, p4, p5, p6
ifundamental            =           p4
inumerator              =           p5
idenominator            =           p6
ivelocity               =           p7
ipan                    =           p8
iratio                  =           inumerator / idenominator
ihertz                  =           ifundamental * iratio
iamp                    =           ampdb(ivelocity)
kenvelope               transeg     0.0, iattack / 2.0, -3.0, iamp / 2.0, iattack / 2.0, 3.0, iamp, isustain, 0.0, iamp, idecay / 2.0, 3.0, iamp / 2.0, idecay / 2.0, -3.0, 0.0
asignal                 poscil3     kenvelope, ihertz, 1
asignal                 chebyshevpoly     asignal, gkChebyshevpoly1, gkChebyshevpoly2, gkChebyshevpoly3
asignal                 dcblock     asignal
aleft, aright           pan2        asignal , ipan
gareverb1               =           gareverb1 + aleft
gareverb2               =           gareverb1 + aright
                        endin
                        
                        
instr 30
aleft, aright           reverbsc    gareverb1, gareverb2, gkReverbscFeedback, 15000.0
aleft                   =           gkMasterLevel * (gareverb1 + aleft * 0.8)
aright                  =           gkMasterLevel * (gareverb2 + aright * 0.8)
                        outs        aleft, aright
gareverb1               =           0
gareverb2               =           0
endin
        '''
        self.score = '''\
;                       A few harmonics...
f   1       0           8193  	    10   	3   0   1   0   0   2 
;                       Change the tempo, if you like.
t   0      30

;   p1      p2      p3          p4              p5         p6           p7        p8
;   insno   onset   duration    fundamental     numerator  denominator  velocity  pan

i   1       0       60          60              1          1            60       -0.875
i   1       0      180          60              3          2            60        0.000
i   1       0       60          60             28         15            60        0.875

i   1       60      60          60              9          8            60        0.875
i   1       60      30          60              10         7            64       -0.875
i   1       90      30          60              4          3            68        0.875

i   1       120     60          60          	1          1            58       -0.875
i   1       120     60          60          	5          4            60        0.000
i   1       120     60          60         	2          1            54        0.875

i   30      0       -1

s   10.0
e   10.0        
        '''
        self.command = None
        self.gkHarmonicTableFactor = 0.5
        self.gkChebyshevpoly1 = 1.25
        self.gkChebyshevpoly2 = 0.125
        self.gkChebyshevpoly3 = 0.0125
        self.gkReverbscFeedback = 0.925
        self.gkMasterLevel = 0.125
        self.gkHarmonicTableFactorScale = 100000.0
        self.gkChebyshevpoly1Scale = 100000.0
        self.gkChebyshevpoly2Scale = 1000000.0
        self.gkChebyshevpoly3Scale = 10000000.0
        self.gkReverbscFeedbackScale = 1000000.0
        self.gkMasterLevelScale = 1000000.0
        self.output = 'dac'
        self.player = soundfilePlayer

import wx
import wx.lib.stattext
import wx.media
import wx.gizmos
import wx.lib.filebrowsebutton
import wx.stc
from wx.lib.anchors import LayoutAnchors

def create(parent):
    return PythonDemoFrame(parent)

[wxID_PYTHONDEMOFRAME, wxID_PYTHONDEMOFRAMEABOUTTEXT, 
 wxID_PYTHONDEMOFRAMEBUTTON1, wxID_PYTHONDEMOFRAMEDELETESNAPSHOTBUTTON, 
 wxID_PYTHONDEMOFRAMEGAINPANEL, wxID_PYTHONDEMOFRAMEGENSTATICTEXT1, 
 wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY1, 
 wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY1TEXT, 
 wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY2, 
 wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY2TEXT, 
 wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY3, 
 wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY3TEXT, 
 wxID_PYTHONDEMOFRAMEGKHARMONICTABLEFACTOR, 
 wxID_PYTHONDEMOFRAMEGKHARMONICTABLEFACTORTEXT, 
 wxID_PYTHONDEMOFRAMEGKMASTERLEVEL, wxID_PYTHONDEMOFRAMEGKMASTERLEVELTEXT, 
 wxID_PYTHONDEMOFRAMEGKREVERBSCFEEDBACK, 
 wxID_PYTHONDEMOFRAMEGKREVERBSCFEEDBACKTEXT, 
 wxID_PYTHONDEMOFRAMEHARMONICSPANEL, wxID_PYTHONDEMOFRAMENOTEBOOK, 
 wxID_PYTHONDEMOFRAMEORCHESTRATEXT, wxID_PYTHONDEMOFRAMEOUTPUTCOMBOBOX, 
 wxID_PYTHONDEMOFRAMEOUTPUTPANEL, wxID_PYTHONDEMOFRAMEPANEL1, 
 wxID_PYTHONDEMOFRAMEPANEL2, wxID_PYTHONDEMOFRAMEPANEL3, 
 wxID_PYTHONDEMOFRAMEPANEL5, wxID_PYTHONDEMOFRAMEPLAYBUTTON, 
 wxID_PYTHONDEMOFRAMEPLAYERBROWSER, wxID_PYTHONDEMOFRAMEPLAYPANEL, 
 wxID_PYTHONDEMOFRAMERENDERBUTTON, wxID_PYTHONDEMOFRAMEREVERBPANEL, 
 wxID_PYTHONDEMOFRAMESAVENEWSNAPSHOTBUTTON, 
 wxID_PYTHONDEMOFRAMESAVESNAPSHOTBUTTON, wxID_PYTHONDEMOFRAMESCORETEXT, 
 wxID_PYTHONDEMOFRAMESNAPSHOTCOMBOBOX, wxID_PYTHONDEMOFRAMESNAPSHOTPANEL, 
 wxID_PYTHONDEMOFRAMESTATICTEXT1, wxID_PYTHONDEMOFRAMESTATICTEXT2, 
 wxID_PYTHONDEMOFRAMESTATICTEXT3, wxID_PYTHONDEMOFRAMESTATICTEXT4, 
 wxID_PYTHONDEMOFRAMESTATICTEXT5, wxID_PYTHONDEMOFRAMESTATICTEXT6, 
 wxID_PYTHONDEMOFRAMESTATICTEXT7, wxID_PYTHONDEMOFRAMESTATICTEXT8, 
 wxID_PYTHONDEMOFRAMESTATICTEXT9, wxID_PYTHONDEMOFRAMEWAVESHAPINGPANEL, 
] = [wx.NewId() for _init_ctrls in range(47)]

[wxID_PYTHONDEMOFRAMETOOLBAR1TOOLS0, wxID_PYTHONDEMOFRAMETOOLBAR1TOOLS1, 
 wxID_PYTHONDEMOFRAMETOOLBAR1TOOLS2, 
] = [wx.NewId() for _init_coll_toolBar1_Tools in range(3)]

class PythonDemoFrame(wx.Frame):
    def _init_coll_notebook_Pages(self, parent):
        # generated method, don't edit

        parent.AddPage(imageId=-1, page=self.orchestraText, select=False,
              text=u'Orchestra')
        parent.AddPage(imageId=-1, page=self.scoreText, select=False,
              text=u'Score')
        parent.AddPage(imageId=-1, page=self.playPanel, select=True,
              text=u'Play')
        parent.AddPage(imageId=-1, page=self.aboutText, select=False,
              text=u'About')

    def _init_ctrls(self, prnt):
        # generated method, don't edit
        wx.Frame.__init__(self, id=wxID_PYTHONDEMOFRAME,
              name=u'PythonDemoFrame', parent=prnt, pos=wx.Point(431, 236),
              size=wx.Size(805, 389), style=wx.DEFAULT_FRAME_STYLE,
              title=u'Csound Python Demo')
        self.SetClientSize(wx.Size(797, 362))
        self.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.NORMAL, False,
              u'Terminal'))
        self.Bind(wx.EVT_CLOSE, self.OnPythonDemoFrameClose)

        self.notebook = wx.Notebook(id=wxID_PYTHONDEMOFRAMENOTEBOOK,
              name=u'notebook', parent=self, pos=wx.Point(0, 0),
              size=wx.Size(797, 362), style=0)
        self.notebook.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.NORMAL, False,
              u'MS Shell Dlg 2'))

        self.playPanel = wx.Panel(id=wxID_PYTHONDEMOFRAMEPLAYPANEL,
              name=u'playPanel', parent=self.notebook, pos=wx.Point(0, 0),
              size=wx.Size(789, 336), style=wx.TAB_TRAVERSAL)

        self.outputPanel = wx.Panel(id=wxID_PYTHONDEMOFRAMEOUTPUTPANEL,
              name=u'outputPanel', parent=self.playPanel, pos=wx.Point(400,
              160), size=wx.Size(384, 120),
              style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)
        self.outputPanel.SetLabel(u'Wavetable high harmonics')

        self.staticText1 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT1,
              label=u'Rendering', name='staticText1', parent=self.outputPanel,
              pos=wx.Point(8, 8), size=wx.Size(58, 13), style=0)
        self.staticText1.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))

        self.harmonicsPanel = wx.Panel(id=wxID_PYTHONDEMOFRAMEHARMONICSPANEL,
              name=u'harmonicsPanel', parent=self.playPanel, pos=wx.Point(8, 8),
              size=wx.Size(384, 56), style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)
        self.harmonicsPanel.SetLabel(u'Wavetable high harmonics')

        self.gkHarmonicTableFactorText = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEGKHARMONICTABLEFACTORTEXT,
              name=u'gkHarmonicTableFactorText', parent=self.harmonicsPanel,
              pos=wx.Point(280, 24), size=wx.Size(92, 21), style=0, value=u'')

        self.staticText2 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT2,
              label=u'Wavetable high harmonics multiplier', name='staticText2',
              parent=self.harmonicsPanel, pos=wx.Point(8, 8), size=wx.Size(208,
              13), style=0)
        self.staticText2.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))

        self.waveshapingPanel = wx.Panel(id=wxID_PYTHONDEMOFRAMEWAVESHAPINGPANEL,
              name=u'waveshapingPanel', parent=self.playPanel, pos=wx.Point(8,
              72), size=wx.Size(384, 192),
              style=wx.CAPTION | wx.RAISED_BORDER | wx.TAB_TRAVERSAL)
        self.waveshapingPanel.SetLabel(u'Waveshaping distortion')

        self.reverbPanel = wx.Panel(id=wxID_PYTHONDEMOFRAMEREVERBPANEL,
              name=u'reverbPanel', parent=self.playPanel, pos=wx.Point(8, 272),
              size=wx.Size(384, 56), style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)
        self.reverbPanel.SetLabel(u'Wavetable high harmonics')

        self.gkReverbscFeedback = wx.Slider(id=wxID_PYTHONDEMOFRAMEGKREVERBSCFEEDBACK,
              maxValue=1000000, minValue=0, name=u'gkReverbscFeedback',
              parent=self.reverbPanel, pos=wx.Point(0, 24), size=wx.Size(272,
              32), style=wx.SL_HORIZONTAL, value=0)
        self.gkReverbscFeedback.SetMax(1000000)
        self.gkReverbscFeedback.Bind(wx.EVT_SCROLL,
              self.on_gkReverbscFeedbackScroll)

        self.gkReverbscFeedbackText = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEGKREVERBSCFEEDBACKTEXT,
              name=u'gkReverbscfeedbackText', parent=self.reverbPanel,
              pos=wx.Point(280, 24), size=wx.Size(92, 21), style=0, value=u'')

        self.staticText4 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT4,
              label=u'Reverberation feedback (delay time)', name='staticText4',
              parent=self.reverbPanel, pos=wx.Point(8, 8), size=wx.Size(210,
              13), style=0)
        self.staticText4.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))

        self.gainPanel = wx.Panel(id=wxID_PYTHONDEMOFRAMEGAINPANEL,
              name=u'gainPanel', parent=self.playPanel, pos=wx.Point(400, 8),
              size=wx.Size(384, 56), style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)
        self.gainPanel.SetLabel(u'Wavetable high harmonics')

        self.gkMasterLevel = wx.Slider(id=wxID_PYTHONDEMOFRAMEGKMASTERLEVEL,
              maxValue=1000000, minValue=0, name=u'gkMasterLevel',
              parent=self.gainPanel, pos=wx.Point(0, 24), size=wx.Size(272, 32),
              style=wx.SL_HORIZONTAL, value=0)
        self.gkMasterLevel.SetMax(1000000)
        self.gkMasterLevel.Bind(wx.EVT_SCROLL, self.on_gkMasterLevelScroll)

        self.gkMasterLevelText = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEGKMASTERLEVELTEXT,
              name=u'gkMasterLevelText', parent=self.gainPanel,
              pos=wx.Point(280, 24), size=wx.Size(92, 21), style=0, value=u'')

        self.staticText5 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT5,
              label=u'Master gain', name='staticText5', parent=self.gainPanel,
              pos=wx.Point(8, 8), size=wx.Size(67, 13), style=0)
        self.staticText5.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))

        self.panel1 = wx.Panel(id=wxID_PYTHONDEMOFRAMEPANEL1, name='panel1',
              parent=self.waveshapingPanel, pos=wx.Point(0, 128),
              size=wx.Size(384, 56), style=wx.TAB_TRAVERSAL)
        self.panel1.SetLabel(u'Wavetable high harmonics')

        self.gkChebyshevpoly3 = wx.Slider(id=wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY3,
              maxValue=1000000, minValue=0, name=u'gkChebyshevpoly3',
              parent=self.panel1, pos=wx.Point(0, 24), size=wx.Size(272, 32),
              style=wx.SL_HORIZONTAL, value=0)
        self.gkChebyshevpoly3.SetMax(1000000)
        self.gkChebyshevpoly3.Bind(wx.EVT_SCROLL,
              self.on_gkChebyshevpoly3Scroll)

        self.staticText3 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT3,
              label=u'Chebyshev polynomial coefficient 3', name='staticText3',
              parent=self.panel1, pos=wx.Point(8, 8), size=wx.Size(169, 13),
              style=0)

        self.gkChebyshevpoly3Text = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY3TEXT,
              name=u'gkChebyshevpoly3Text', parent=self.panel1,
              pos=wx.Point(280, 24), size=wx.Size(92, 21), style=0, value=u'')

        self.panel2 = wx.Panel(id=wxID_PYTHONDEMOFRAMEPANEL2, name='panel2',
              parent=self.waveshapingPanel, pos=wx.Point(0, 32),
              size=wx.Size(384, 56), style=wx.TAB_TRAVERSAL)
        self.panel2.SetLabel(u'Wavetable high harmonics')

        self.gkChebyshevpoly1 = wx.Slider(id=wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY1,
              maxValue=1000000, minValue=0, name=u'gkChebyshevpoly1',
              parent=self.panel2, pos=wx.Point(0, 24), size=wx.Size(272, 32),
              style=wx.SL_HORIZONTAL, value=0)
        self.gkChebyshevpoly1.SetMax(1000000)
        self.gkChebyshevpoly1.Bind(wx.EVT_SCROLL,
              self.on_gkChebyshevpoly1Scroll)

        self.staticText6 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT6,
              label=u'Chebyshev polynomial coefficient 1', name='staticText6',
              parent=self.panel2, pos=wx.Point(8, 8), size=wx.Size(169, 13),
              style=0)

        self.gkChebyshevpoly1Text = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY1TEXT,
              name=u'gkChebyshevpoly1Text', parent=self.panel2,
              pos=wx.Point(280, 24), size=wx.Size(92, 21), style=0, value=u'')

        self.staticText7 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT7,
              label=u'Waveshaping distortion', name='staticText7',
              parent=self.waveshapingPanel, pos=wx.Point(8, 8),
              size=wx.Size(135, 13), style=0)
        self.staticText7.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))

        self.panel5 = wx.Panel(id=wxID_PYTHONDEMOFRAMEPANEL5, name='panel5',
              parent=self.waveshapingPanel, pos=wx.Point(0, 80),
              size=wx.Size(384, 56), style=wx.TAB_TRAVERSAL)
        self.panel5.SetLabel(u'Wavetable high harmonics')

        self.gkChebyshevpoly2 = wx.Slider(id=wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY2,
              maxValue=1000000, minValue=0, name=u'gkChebyshevpoly2',
              parent=self.panel5, pos=wx.Point(0, 24), size=wx.Size(272, 32),
              style=wx.SL_HORIZONTAL, value=0)
        self.gkChebyshevpoly2.SetMax(1000000)
        self.gkChebyshevpoly2.Bind(wx.EVT_SCROLL,
              self.on_gkChebyshevpoly2Scroll)

        self.staticText8 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT8,
              label=u'Chebyshev polynomial coefficient 2', name='staticText8',
              parent=self.panel5, pos=wx.Point(8, 8), size=wx.Size(169, 13),
              style=0)

        self.gkChebyshevpoly2Text = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEGKCHEBYSHEVPOLY2TEXT,
              name=u'gkChebyshevpoly2Text', parent=self.panel5,
              pos=wx.Point(280, 24), size=wx.Size(92, 21), style=0, value=u'')

        self.gkHarmonicTableFactor = wx.Slider(id=wxID_PYTHONDEMOFRAMEGKHARMONICTABLEFACTOR,
              maxValue=1000000, minValue=0, name=u'gkHarmonicTableFactor',
              parent=self.harmonicsPanel, pos=wx.Point(0, 24), size=wx.Size(272,
              24), style=wx.SL_HORIZONTAL, value=0)
        self.gkHarmonicTableFactor.Bind(wx.EVT_SCROLL,
              self.on_gkHarmonicTableFactorScroll)

        self.snapshotPanel = wx.Panel(id=wxID_PYTHONDEMOFRAMESNAPSHOTPANEL,
              name=u'snapshotPanel', parent=self.playPanel, pos=wx.Point(400,
              72), size=wx.Size(384, 80),
              style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)

        self.staticText9 = wx.StaticText(id=wxID_PYTHONDEMOFRAMESTATICTEXT9,
              label=u'Settings snapshots', name='staticText9',
              parent=self.snapshotPanel, pos=wx.Point(8, 8), size=wx.Size(108,
              13), style=0)
        self.staticText9.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))

        self.saveNewSnapshotButton = wx.Button(id=wxID_PYTHONDEMOFRAMESAVENEWSNAPSHOTBUTTON,
              label=u'New', name=u'saveNewSnapshotButton',
              parent=self.snapshotPanel, pos=wx.Point(8, 32), size=wx.Size(64,
              32), style=0)
        self.saveNewSnapshotButton.Bind(wx.EVT_BUTTON,
              self.OnSaveNewSnapshotButtonButton,
              id=wxID_PYTHONDEMOFRAMESAVENEWSNAPSHOTBUTTON)

        self.deleteSnapshotButton = wx.Button(id=wxID_PYTHONDEMOFRAMEDELETESNAPSHOTBUTTON,
              label=u'Delete', name=u'deleteSnapshotButton',
              parent=self.snapshotPanel, pos=wx.Point(152, 32), size=wx.Size(64,
              32), style=0)
        self.deleteSnapshotButton.SetToolTipString(u'Save snapshot')
        self.deleteSnapshotButton.Bind(wx.EVT_BUTTON,
              self.OnDeleteSnapshotButtonButton,
              id=wxID_PYTHONDEMOFRAMEDELETESNAPSHOTBUTTON)

        self.renderButton = wx.Button(id=wxID_PYTHONDEMOFRAMERENDERBUTTON,
              label=u'Render', name=u'renderButton', parent=self.outputPanel,
              pos=wx.Point(160, 32), size=wx.Size(64, 32), style=0)
        self.renderButton.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD,
              False, u'MS Shell Dlg 2'))
        self.renderButton.SetForegroundColour(wx.Colour(0, 128, 128))
        self.renderButton.Bind(wx.EVT_BUTTON, self.OnRenderButtonButton,
              id=wxID_PYTHONDEMOFRAMERENDERBUTTON)

        self.playButton = wx.Button(id=wxID_PYTHONDEMOFRAMEPLAYBUTTON,
              label=u'Edit', name=u'playButton', parent=self.outputPanel,
              pos=wx.Point(304, 32), size=wx.Size(64, 32), style=0)
        self.playButton.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))
        self.playButton.SetForegroundColour(wx.Colour(0, 128, 255))
        self.playButton.Bind(wx.EVT_BUTTON, self.OnPlayButtonButton,
              id=wxID_PYTHONDEMOFRAMEPLAYBUTTON)

        self.button1 = wx.Button(id=wxID_PYTHONDEMOFRAMEBUTTON1, label=u'Stop',
              name='button1', parent=self.outputPanel, pos=wx.Point(232, 32),
              size=wx.Size(64, 32), style=0)
        self.button1.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD, False,
              u'MS Shell Dlg 2'))
        self.button1.SetForegroundColour(wx.Colour(255, 0, 0))
        self.button1.Bind(wx.EVT_BUTTON, self.OnStopButtonButton,
              id=wxID_PYTHONDEMOFRAMEBUTTON1)

        self.panel3 = wx.Panel(id=wxID_PYTHONDEMOFRAMEPANEL3, name='panel3',
              parent=self.outputPanel, pos=wx.Point(8, 72), size=wx.Size(360,
              32), style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)

        self.playerBrowser = wx.lib.filebrowsebutton.FileBrowseButton(buttonText='Browse',
              dialogTitle='Choose a file', fileMask=u'*.*',
              id=wxID_PYTHONDEMOFRAMEPLAYERBROWSER, initialValue=u'sndrec.exe',
              labelText=u'Player', parent=self.panel3, pos=wx.Point(8, 0),
              size=wx.Size(344, 24), startDirectory='.', style=wx.TAB_TRAVERSAL,
              toolTip='Type filename or click browse to choose file')
        self.playerBrowser.SetName(u'playerBrowser')

        self.aboutText = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEABOUTTEXT,
              name=u'aboutText', parent=self.notebook, pos=wx.Point(0, 0),
              size=wx.Size(789, 336),
              style=wx.TE_READONLY | wx.TE_LINEWRAP | wx.TE_MULTILINE,
              value=u"This application demonstrates the use of Boa Constructor to build a Python program with a graphical user interface using the wxPython toolkit.\n\nThe purpose of this application is not so much real-time performance, as to enable the composer to 'fine-tune' parameters of the composition that have a global effect on the sound.\n\nThe application communicates with Csound via Csound's Python application programming interface (API). Csound runs in a separate thread using a csnd6.CsoundPerformandeThread object, and the sliders on the user interface control global variables in the Csound orchestra during performance, using the channel API.\n\nThe application also provides for taking any number of 'snapshots' of the control settings, including the Csound orchestra and score, and saving or restoring the settings.\n\nYou can adapt this application to use for your own compositions by copying the files to a new directory, opening them with Boa Constructor, and editing the application.")
        self.aboutText.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.NORMAL, False,
              u'Courier New'))

        self.orchestraText = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMEORCHESTRATEXT,
              name=u'orchestraText', parent=self.notebook, pos=wx.Point(0, 0),
              size=wx.Size(789, 336),
              style=wx.TE_DONTWRAP | wx.TE_LINEWRAP | wx.TE_WORDWRAP | wx.TE_MULTILINE,
              value=u'')
        self.orchestraText.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.NORMAL,
              False, u'Courier New'))

        self.scoreText = wx.TextCtrl(id=wxID_PYTHONDEMOFRAMESCORETEXT,
              name=u'scoreText', parent=self.notebook, pos=wx.Point(0, 0),
              size=wx.Size(789, 336),
              style=wx.TE_DONTWRAP | wx.TE_LINEWRAP | wx.TE_WORDWRAP | wx.TE_MULTILINE,
              value=u'')
        self.scoreText.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.NORMAL, False,
              u'Courier New'))

        self.snapshotComboBox = wx.ComboBox(choices=[],
              id=wxID_PYTHONDEMOFRAMESNAPSHOTCOMBOBOX, name=u'snapshotComboBox',
              parent=self.snapshotPanel, pos=wx.Point(280, 32), size=wx.Size(88,
              21), style=0, value=u'1')
        self.snapshotComboBox.SetLabel(u'1')
        self.snapshotComboBox.Bind(wx.EVT_COMBOBOX,
              self.OnSnapshotComboBoxCombobox,
              id=wxID_PYTHONDEMOFRAMESNAPSHOTCOMBOBOX)

        self.saveSnapshotButton = wx.Button(id=wxID_PYTHONDEMOFRAMESAVESNAPSHOTBUTTON,
              label=u'Save', name=u'saveSnapshotButton',
              parent=self.snapshotPanel, pos=wx.Point(80, 32), size=wx.Size(64,
              32), style=0)
        self.saveSnapshotButton.SetToolTipString(u'Save snapshot')
        self.saveSnapshotButton.Bind(wx.EVT_BUTTON,
              self.OnSaveSnapshotButtonButton,
              id=wxID_PYTHONDEMOFRAMESAVESNAPSHOTBUTTON)

        self.genStaticText1 = wx.lib.stattext.GenStaticText(ID=wxID_PYTHONDEMOFRAMEGENSTATICTEXT1,
              label=u'Output', name='genStaticText1', parent=self.outputPanel,
              pos=wx.Point(8, 32), size=wx.Size(34, 13), style=0)

        self.outputComboBox = wx.ComboBox(choices=[],
              id=wxID_PYTHONDEMOFRAMEOUTPUTCOMBOBOX, name=u'outputComboBox',
              parent=self.outputPanel, pos=wx.Point(56, 32), size=wx.Size(96,
              21), style=0, value=u'dac')
        self.outputComboBox.SetLabel(u'dac')
        self.outputComboBox.Bind(wx.EVT_COMBOBOX, self.OnOutputComboBoxCombobox,
              id=wxID_PYTHONDEMOFRAMEOUTPUTCOMBOBOX)

        self._init_coll_notebook_Pages(self.notebook)

    def __init__(self, parent):
        self._init_ctrls(parent)
        try:
            self.csound = csnd6.CppSound()
            self.slidersInitialized = False
            self.configuration = Configuration()
            self.csoundCommands = {
	'dac':      'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac',         orcFilename, scoFilename),
	'dac0':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac0',        orcFilename, scoFilename),
	'dac1':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac1',        orcFilename, scoFilename),
	'dac2':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac2',        orcFilename, scoFilename),
	'dac3':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac3',        orcFilename, scoFilename),
	'dac4':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac4',        orcFilename, scoFilename),
	'dac5':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac5',        orcFilename, scoFilename),
	'dac6':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac6',        orcFilename, scoFilename),
	'dac7':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac7',        orcFilename, scoFilename),
	'dac8':     'csound --messagelevel=1 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s %s %s' % (			           'dac8',        orcFilename, scoFilename),
	'Preview':  'csound --messagelevel=3 -W -f --rewrite --dither --nopeaks --nodisplays --sample-rate=44100 --control-rate=441   --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s --output=%s %s %s' % (author, author, title, soundfileName, orcFilename, scoFilename),
	'CD':       'csound --messagelevel=3 -W -f --rewrite --dither --nopeaks --nodisplays --sample-rate=44100 --control-rate=44100 --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s --output=%s %s %s' % (author, author, title, soundfileName, orcFilename, scoFilename),
	'Master':   'csound --messagelevel=3 -W -f --rewrite --dither --nopeaks --nodisplays --sample-rate=88200 --control-rate=88200 --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s --output=%s %s %s' % (author, author, title, soundfileName, orcFilename, scoFilename)}
            outputs = self.csoundCommands.keys()
            outputs.sort()
            for output in outputs:
                self.outputComboBox.Append(output)
            self.snapshots = []
            self.snapshots.append(self.configuration)
            self.createSnapshotNumbers()
            self.loadSnapshots()
            self.updateView()
        except:
            traceback.print_exc()
            
    def updateModel(self):
        try:
            self.configuration.orchestra = str(self.orchestraText.GetValue())
            self.configuration.score = str(self.scoreText.GetValue())
            self.configuration.command = str(self.csoundCommands[self.outputComboBox.GetValue()])
            self.configuration.gkHarmonicTableFactor = float(self.gkHarmonicTableFactor.GetValue()) / self.configuration.gkHarmonicTableFactorScale
            self.csound.SetChannel('gkHarmonicTableFactor', self.configuration.gkHarmonicTableFactor)
            self.configuration.gkChebyshevpoly1 = float(self.gkChebyshevpoly1.GetValue()) / self.configuration.gkChebyshevpoly1Scale
            self.csound.SetChannel('gkChebyshevpoly1', self.configuration.gkChebyshevpoly1)
            self.configuration.gkChebyshevpoly2 = float(self.gkChebyshevpoly2.GetValue()) / self.configuration.gkChebyshevpoly2Scale
            self.csound.SetChannel('gkChebyshevpoly2', self.configuration.gkChebyshevpoly2)
            self.configuration.gkChebyshevpoly3 = float(self.gkChebyshevpoly3.GetValue()) / self.configuration.gkChebyshevpoly3Scale
            self.csound.SetChannel('gkChebyshevpoly3', self.configuration.gkChebyshevpoly3)
            self.configuration.gkReverbscFeedback = float(self.gkReverbscFeedback.GetValue()) / self.configuration.gkReverbscFeedbackScale
            self.csound.SetChannel('gkReverbscFeedback', self.configuration.gkReverbscFeedback)
            self.configuration.gkMasterLevel = float(self.gkMasterLevel.GetValue()) / self.configuration.gkMasterLevelScale
            self.csound.SetChannel('gkMasterLevel', self.configuration.gkMasterLevel)
            self.configuration.player = str(self.playerBrowser.GetValue())
        except:
            traceback.print_exc()
        
    def updateView(self):
        try:
            self.orchestraText.SetValue(str(self.configuration.orchestra))
            self.scoreText.SetValue(str(self.configuration.score))
            self.gkHarmonicTableFactor.SetValue(int(self.configuration.gkHarmonicTableFactor * self.configuration.gkHarmonicTableFactorScale))
            self.gkHarmonicTableFactorText.SetValue(str(self.configuration.gkHarmonicTableFactor))
            self.gkChebyshevpoly1.SetValue(int(self.configuration.gkChebyshevpoly1 * self.configuration.gkChebyshevpoly1Scale))
            self.gkChebyshevpoly1Text.SetValue(str(self.configuration.gkChebyshevpoly1))
            self.gkChebyshevpoly2.SetValue(int(self.configuration.gkChebyshevpoly2 * self.configuration.gkChebyshevpoly2Scale))
            self.gkChebyshevpoly2Text.SetValue(str(self.configuration.gkChebyshevpoly2))
            self.gkChebyshevpoly3.SetValue(int(self.configuration.gkChebyshevpoly3 * self.configuration.gkChebyshevpoly3Scale))
            self.gkChebyshevpoly3Text.SetValue(str(self.configuration.gkChebyshevpoly3))
            self.gkReverbscFeedback.SetValue(int(self.configuration.gkReverbscFeedback * self.configuration.gkReverbscFeedbackScale))
            self.gkReverbscFeedbackText.SetValue(str(self.configuration.gkReverbscFeedback))
            self.gkMasterLevel.SetValue(int(self.configuration.gkMasterLevel * self.configuration.gkMasterLevelScale))
            self.gkMasterLevelText.SetValue(str(self.configuration.gkMasterLevel))
            self.outputComboBox.SetValue(self.configuration.output)
            self.playerBrowser.SetValue(self.configuration.player)
        except:
            traceback.print_exc()

    def createSnapshotNumbers(self):
        snapshotIndex = self.getSnapshotIndex()
        self.snapshotComboBox.Clear()
        for i in xrange(1, len(self.snapshots) + 1):
            self.snapshotComboBox.Append(str(i))
        if len(self.snapshots) == 1:
            self.snapshotComboBox.SetValue('1')
        elif 0 <= snapshotIndex and snapshotIndex < len(self.snapshots):
            self.snapshotComboBox.SetValue(str(snapshotIndex + 1))
            
    def getSnapshotIndex(self):
        return int(self.snapshotComboBox.GetValue()) - 1
    
    def loadSnapshots(self):
        snapshots = None
        try:
			picklefile = open(snapshotsFilename, 'rb')
			snapshots = pickle.load(picklefile)
			picklefile.close()
        except:
            traceback.print_exc()
            return
        if snapshots:
            self.snapshots = snapshots
            self.createSnapshotNumbers()
            self.snapshotComboBox.SetValue(str(len(self.snapshots)))
            self.restoreSnapshot()
            print 'Loaded configuration: "%s".' % snapshotsFilename
    
    def saveSnapshots(self):
        try:
            picklefile = open(snapshotsFilename, 'wb')
            pickle.dump(self.snapshots, picklefile)
            picklefile.close()            
            print 'Saved configuration: "%s".' % snapshotsFilename
        except:
            traceback.print_exc()
            
    def restoreSnapshot(self):
        try:
            self.configuration = copy.deepcopy(self.snapshots[self.getSnapshotIndex()])
            self.updateView()
        except:
            traceback.print_exc()
    
    def saveSnapshot(self):
        try:
            self.updateModel()
            self.snapshots[self.getSnapshotIndex()] = copy.deepcopy(self.configuration)
            self.saveSnapshots()
        except:
            traceback.print_exc()
        
    def newSnapshot(self):
        try:
            self.updateModel()
            self.snapshots.append(copy.deepcopy(self.configuration))
            self.createSnapshotNumbers()
            self.snapshotComboBox.SetValue(str(len(self.snapshots)))
            self.saveSnapshots()
        except:
            traceback.print_exc()
        
    def deleteSnapshot(self):
        try:
            del self.snapshots[self.getSnapshotIndex()]
            self.createSnapshotNumbers()
            self.restoreSnapshot()
            self.saveSnapshots()
        except:
            traceback.print_exc()
                   
    def playThreadRoutine(self):
        try:
            print 'playThreadRoutine...'
            self.updateModel()
            self.csound.setOrchestra(self.configuration.orchestra)
            self.csound.setScore(self.configuration.score)
            self.csound.setCommand(self.configuration.command)
            self.csound.exportForPerformance()
            self.csound.compile()
            self.updateModel()
            self.csoundPerformanceThread = csnd6.CsoundPerformanceThread(self.csound)
            self.csoundPerformanceThread.Play()
            print 'Playing...'
        except:
            traceback.print_exc()

    def OnPlayButtonButton(self, event):
        try:
            print 'OnPlayButtonButton...'
            print 'OPENING SOUNDFILE FOR PLAYBACK...'
            print
            print 'Player: %s  Soundfile:             "%s"' % (self.configuration.player, soundfileName)
            os.spawnl(os.P_NOWAIT, self.configuration.player, self.configuration.player, soundfileName)
            print
        except:
            print traceback.print_exc()
            									
    def OnStopButtonButton(self, event):
        try:
            print 'Ending performance...'
            self.csoundPerformanceThread.Stop()
            self.csoundPerformanceThread.Join()
            self.csoundThread.join()
            print 'Csound thread has finished.'
        except:
            print traceback.print_exc()
            
    def on_gkHarmonicTableFactorScroll(self, event):
        try:
            self.configuration.gkHarmonicTableFactor = float(event.GetPosition()) / self.configuration.gkHarmonicTableFactorScale
            self.gkHarmonicTableFactorText.SetValue(str(self.configuration.gkHarmonicTableFactor))
            message = 'f  1  0 	8193  10  3.0  0  %f  0  0 %f\n' % (self.configuration.gkHarmonicTableFactor * 1.0, self.configuration.gkHarmonicTableFactor * 2.0)
            self.csound.inputMessage(message)
        except:
            print traceback.print_exc()
        
    def on_gkReverbscFeedbackScroll(self, event):
        try:
            self.configuration.gkReverbscFeedback = float(event.GetPosition()) / self.configuration.gkReverbscFeedbackScale
            self.gkReverbscFeedbackText.SetValue(str(self.configuration.gkReverbscFeedback))
            self.csound.SetChannel('gkReverbscFeedback', self.configuration.gkReverbscFeedback)
        except:
            traceback.print_exc()

    def on_gkMasterLevelScroll(self, event):
        try:
            self.configuration.gkMasterLevel = float(event.GetPosition()) / self.configuration.gkMasterLevelScale
            self.gkMasterLevelText.SetValue(str(self.configuration.gkMasterLevel))
            self.csound.SetChannel('gkMasterLevel', self.configuration.gkMasterLevel)
        except:
            traceback.print_exc()

    def on_gkChebyshevpoly3Scroll(self, event):
        try:
            self.configuration.gkChebyshevpoly3 = float(event.GetPosition()) / self.configuration.gkChebyshevpoly3Scale
            self.gkChebyshevpoly3Text.SetValue(str(self.configuration.gkChebyshevpoly3))
            self.csound.SetChannel('gkChebyshevpoly3', self.configuration.gkChebyshevpoly3)
        except:
            traceback.print_exc()

    def on_gkChebyshevpoly1Scroll(self, event):
        try:
            self.configuration.gkChebyshevpoly1 = float(event.GetPosition()) / self.configuration.gkChebyshevpoly1Scale
            self.gkChebyshevpoly1Text.SetValue(str(self.configuration.gkChebyshevpoly1))
            self.csound.SetChannel('gkChebyshevpoly1', self.configuration.gkChebyshevpoly1)
        except:
            traceback.print_exc()

    def on_gkChebyshevpoly2Scroll(self, event):
        try:
            self.configuration.gkChebyshevpoly2 = float(event.GetPosition()) / self.configuration.gkChebyshevpoly2Scale
            self.gkChebyshevpoly2Text.SetValue(str(self.configuration.gkChebyshevpoly2))
            self.csound.SetChannel('gkChebyshevpoly2', self.configuration.gkChebyshevpoly2)
        except:
            traceback.print_exc()

    def OnRenderButtonButton(self, event):
        try:
            self.csoundThread = threading.Thread(None, self.playThreadRoutine)
            self.csoundThread.start()
        except:
            traceback.print_exc()

    def OnSaveSnapshotButtonButton(self, event):
        try:
            print 'OnSaveSnapshotButtonButton'
            self.saveSnapshot()
        except:
            traceback.print_exc()

    def OnSaveNewSnapshotButtonButton(self, event):
        try:
            print 'OnSaveNewSnapshotButtonButton'
            self.newSnapshot()
        except:
            traceback.print_exc()
 
    def OnOutputComboBoxCombobox(self, event):
        try:
            print 'OnOutputComboBoxCombobox'
        except:
            traceback.print_exc()

    def OnDeleteSnapshotButtonButton(self, event):
        try:
            print 'OnDeleteSnapshotButtonButton'
            self.deleteSnapshot()
        except:
            traceback.print_exc()

    def OnSnapshotComboBoxCombobox(self, event):
        try:
            print 'OnSnapshotComboBoxCombobox'
            self.restoreSnapshot()
        except:
            traceback.print_exc()

    def OnPythonDemoFrameClose(self, event):
        try:
            self.OnStopButtonButton(None)
            event.Skip()
        except:
            traceback.print_exc()

    def OnPythonDemoFrameHelp(self, event):
        event.Skip()
            
 
