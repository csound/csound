#Boa:Frame:MainFrame

import wx
import csnd
import threading
import traceback
import time
import math

def create(parent):
    return MainFrame(parent)

[wxID_MAINFRAME, wxID_MAINFRAMEAUDIOOUTPUTTEXT, 
 wxID_MAINFRAMECIRCUITELEMENTSLABEL, wxID_MAINFRAMECIRCUITPANEL, 
 wxID_MAINFRAMEGII3, wxID_MAINFRAMEGIV1, wxID_MAINFRAMEGIV2, 
 wxID_MAINFRAMEGKC1, wxID_MAINFRAMEGKC2, wxID_MAINFRAMEGKE, 
 wxID_MAINFRAMEGKFILTERFREQUENCY, wxID_MAINFRAMEGKFILTERRESONANCE, 
 wxID_MAINFRAMEGKG, wxID_MAINFRAMEGKGA, wxID_MAINFRAMEGKGB, wxID_MAINFRAMEGKL, 
 wxID_MAINFRAMEGKOUTPUTGAIN, wxID_MAINFRAMEGKR0, wxID_MAINFRAMEGKREVERBDECAY, 
 wxID_MAINFRAMEGKREVERBDRYWETMIX, wxID_MAINFRAMEGKSTEP_SIZE, 
 wxID_MAINFRAMEPANEL1, wxID_MAINFRAMEPANEL10, wxID_MAINFRAMEPANEL11, 
 wxID_MAINFRAMEPANEL12, wxID_MAINFRAMEPANEL13, wxID_MAINFRAMEPANEL14, 
 wxID_MAINFRAMEPANEL15, wxID_MAINFRAMEPANEL16, wxID_MAINFRAMEPANEL17, 
 wxID_MAINFRAMEPANEL18, wxID_MAINFRAMEPANEL19, wxID_MAINFRAMEPANEL2, 
 wxID_MAINFRAMEPANEL20, wxID_MAINFRAMEPANEL21, wxID_MAINFRAMEPANEL22, 
 wxID_MAINFRAMEPANEL3, wxID_MAINFRAMEPANEL4, wxID_MAINFRAMEPANEL5, 
 wxID_MAINFRAMEPANEL6, wxID_MAINFRAMEPANEL7, wxID_MAINFRAMEPANEL8, 
 wxID_MAINFRAMEPANEL9, wxID_MAINFRAMEPLAYBUTTON, wxID_MAINFRAMEPRESETCOMBOBOX, 
 wxID_MAINFRAMEPROCESSINGLABEL, wxID_MAINFRAMEPROCESSINGPANEL, 
 wxID_MAINFRAMESTATICBITMAP1, wxID_MAINFRAMESTATICTEXT1, 
 wxID_MAINFRAMESTATICTEXT10, wxID_MAINFRAMESTATICTEXT11, 
 wxID_MAINFRAMESTATICTEXT12, wxID_MAINFRAMESTATICTEXT13, 
 wxID_MAINFRAMESTATICTEXT14, wxID_MAINFRAMESTATICTEXT15, 
 wxID_MAINFRAMESTATICTEXT16, wxID_MAINFRAMESTATICTEXT17, 
 wxID_MAINFRAMESTATICTEXT18, wxID_MAINFRAMESTATICTEXT19, 
 wxID_MAINFRAMESTATICTEXT2, wxID_MAINFRAMESTATICTEXT20, 
 wxID_MAINFRAMESTATICTEXT21, wxID_MAINFRAMESTATICTEXT3, 
 wxID_MAINFRAMESTATICTEXT4, wxID_MAINFRAMESTATICTEXT5, 
 wxID_MAINFRAMESTATICTEXT6, wxID_MAINFRAMESTATICTEXT7, 
 wxID_MAINFRAMESTATICTEXT8, wxID_MAINFRAMESTATICTEXT9, 
 wxID_MAINFRAMESTOPBUTTON, 
] = [wx.NewId() for _init_ctrls in range(70)]

class MainFrame(wx.Frame):
    def _init_ctrls(self, prnt):
        # generated method, don't edit
        wx.Frame.__init__(self, id=wxID_MAINFRAME, name='MainFrame',
              parent=prnt, pos=wx.Point(761, 19), size=wx.Size(650, 751),
              style=wx.DEFAULT_FRAME_STYLE, title=u"Chua's Oscillator")
        self.SetClientSize(wx.Size(642, 724))
        self.SetBackgroundColour(wx.Colour(128, 128, 128))

        self.panel1 = wx.Panel(id=wxID_MAINFRAMEPANEL1, name='panel1',
              parent=self, pos=wx.Point(0, 0), size=wx.Size(642, 724),
              style=wx.TAB_TRAVERSAL)

        self.staticBitmap1 = wx.StaticBitmap(bitmap=wx.Bitmap(u'D:/utah/home/mkg/projects/csound-mingw-release/examples/chuas_oscillator/Chua.PNG',
              wx.BITMAP_TYPE_PNG), id=wxID_MAINFRAMESTATICBITMAP1,
              name='staticBitmap1', parent=self.panel1, pos=wx.Point(144, 8),
              size=wx.Size(336, 147), style=0)

        self.circuitPanel = wx.Panel(id=wxID_MAINFRAMECIRCUITPANEL,
              name=u'circuitPanel', parent=self.panel1, pos=wx.Point(0, 240),
              size=wx.Size(320, 480),
              style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)
        self.circuitPanel.SetToolTipString(u'Circuit Elements')

        self.circuitElementsLabel = wx.StaticText(id=wxID_MAINFRAMECIRCUITELEMENTSLABEL,
              label=u'Circuit Elements', name=u'circuitElementsLabel',
              parent=self.circuitPanel, pos=wx.Point(8, 8), size=wx.Size(91,
              13), style=0)
        self.circuitElementsLabel.SetToolTipString(u'')
        self.circuitElementsLabel.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL,
              wx.BOLD, False, u'MS Shell Dlg 2'))

        self.processingPanel = wx.Panel(id=wxID_MAINFRAMEPROCESSINGPANEL,
              name=u'processingPanel', parent=self.panel1, pos=wx.Point(320,
              240), size=wx.Size(320, 360),
              style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)
        self.processingPanel.SetToolTipString(u'Circuit Elements')

        self.processingLabel = wx.StaticText(id=wxID_MAINFRAMEPROCESSINGLABEL,
              label=u'Post- Processing', name=u'processingLabel',
              parent=self.processingPanel, pos=wx.Point(8, 8), size=wx.Size(94,
              13), style=0)
        self.processingLabel.SetToolTipString(u'')
        self.processingLabel.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD,
              False, u'MS Shell Dlg 2'))

        self.panel2 = wx.Panel(id=wxID_MAINFRAMEPANEL2, name='panel2',
              parent=self.circuitPanel, pos=wx.Point(0, 432), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel2.SetLabel(u'Inductor L')
        self.panel2.SetHelpText(u'')

        self.staticText2 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT2,
              label=u'Initial I3', name='staticText2', parent=self.panel2,
              pos=wx.Point(8, 8), size=wx.Size(39, 13), style=0)

        self.panel3 = wx.Panel(id=wxID_MAINFRAMEPANEL3, name='panel3',
              parent=self.circuitPanel, pos=wx.Point(0, 32), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel3.SetLabel(u'Inductor L')
        self.panel3.SetHelpText(u'')

        self.staticText1 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT1,
              label=u'Inductor L', name='staticText1', parent=self.panel3,
              pos=wx.Point(8, 8), size=wx.Size(49, 13), style=0)

        self.giI3 = wx.TextCtrl(id=wxID_MAINFRAMEGII3, name=u'giI3',
              parent=self.panel2, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.giI3.Bind(wx.EVT_TEXT, self.OnGiI3Text, id=wxID_MAINFRAMEGII3)

        self.panel4 = wx.Panel(id=wxID_MAINFRAMEPANEL4, name='panel4',
              parent=self.circuitPanel, pos=wx.Point(0, 72), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel4.SetLabel(u'Inductor L')
        self.panel4.SetHelpText(u'')

        self.staticText3 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT3,
              label=u'Resistor R0', name='staticText3', parent=self.panel4,
              pos=wx.Point(8, 8), size=wx.Size(55, 13), style=0)

        self.gkR0 = wx.TextCtrl(id=wxID_MAINFRAMEGKR0, name=u'gkR0',
              parent=self.panel4, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkR0.Bind(wx.EVT_TEXT, self.OnGkR0Text, id=wxID_MAINFRAMEGKR0)

        self.panel5 = wx.Panel(id=wxID_MAINFRAMEPANEL5, name='panel5',
              parent=self.circuitPanel, pos=wx.Point(0, 112), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel5.SetLabel(u'Inductor L')
        self.panel5.SetHelpText(u'')

        self.gkC1 = wx.TextCtrl(id=wxID_MAINFRAMEGKC1, name=u'gkC1',
              parent=self.panel5, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkC1.Bind(wx.EVT_TEXT, self.OnGkC1Text, id=wxID_MAINFRAMEGKC1)

        self.staticText4 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT4,
              label=u'Capacitor C1', name='staticText4', parent=self.panel5,
              pos=wx.Point(8, 8), size=wx.Size(62, 13), style=0)

        self.panel6 = wx.Panel(id=wxID_MAINFRAMEPANEL6, name='panel6',
              parent=self.circuitPanel, pos=wx.Point(0, 152), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel6.SetLabel(u'Inductor L')
        self.panel6.SetHelpText(u'')

        self.gkC2 = wx.TextCtrl(id=wxID_MAINFRAMEGKC2, name=u'gkC2',
              parent=self.panel6, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkC2.Bind(wx.EVT_TEXT, self.OnGkC2Text, id=wxID_MAINFRAMEGKC2)

        self.staticText5 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT5,
              label=u'Capacitor C2', name='staticText5', parent=self.panel6,
              pos=wx.Point(8, 8), size=wx.Size(62, 13), style=0)

        self.panel7 = wx.Panel(id=wxID_MAINFRAMEPANEL7, name='panel7',
              parent=self.circuitPanel, pos=wx.Point(0, 192), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel7.SetLabel(u'Inductor L')
        self.panel7.SetHelpText(u'')

        self.staticText6 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT6,
              label=u'Resistor G', name='staticText6', parent=self.panel7,
              pos=wx.Point(8, 8), size=wx.Size(49, 13), style=0)

        self.gkG = wx.TextCtrl(id=wxID_MAINFRAMEGKG, name=u'gkG',
              parent=self.panel7, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkG.Bind(wx.EVT_TEXT, self.OnGkGText, id=wxID_MAINFRAMEGKG)

        self.panel8 = wx.Panel(id=wxID_MAINFRAMEPANEL8, name='panel8',
              parent=self.circuitPanel, pos=wx.Point(0, 232), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel8.SetLabel(u'Inductor L')
        self.panel8.SetHelpText(u'')

        self.staticText7 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT7,
              label=u'Nonlinearity Ga', name='staticText7', parent=self.panel8,
              pos=wx.Point(8, 8), size=wx.Size(73, 13), style=0)

        self.gkGa = wx.TextCtrl(id=wxID_MAINFRAMEGKGA, name=u'gkGa',
              parent=self.panel8, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkGa.Bind(wx.EVT_TEXT, self.OnGkGaText, id=wxID_MAINFRAMEGKGA)

        self.panel9 = wx.Panel(id=wxID_MAINFRAMEPANEL9, name='panel9',
              parent=self.circuitPanel, pos=wx.Point(0, 272), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel9.SetLabel(u'Inductor L')
        self.panel9.SetHelpText(u'')

        self.gkGb = wx.TextCtrl(id=wxID_MAINFRAMEGKGB, name=u'gkGb',
              parent=self.panel9, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkGb.Bind(wx.EVT_TEXT, self.OnGkGbText, id=wxID_MAINFRAMEGKGB)

        self.staticText8 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT8,
              label=u'Nonlinearity Gb', name='staticText8', parent=self.panel9,
              pos=wx.Point(8, 8), size=wx.Size(73, 13), style=0)

        self.panel10 = wx.Panel(id=wxID_MAINFRAMEPANEL10, name='panel10',
              parent=self.circuitPanel, pos=wx.Point(0, 312), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel10.SetLabel(u'Inductor L')
        self.panel10.SetHelpText(u'')

        self.staticText9 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT9,
              label=u'Nonlinearity E', name='staticText9', parent=self.panel10,
              pos=wx.Point(8, 8), size=wx.Size(66, 13), style=0)

        self.gkE = wx.TextCtrl(id=wxID_MAINFRAMEGKE, name=u'gkE',
              parent=self.panel10, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkE.Bind(wx.EVT_TEXT, self.OnGkEText, id=wxID_MAINFRAMEGKE)

        self.panel11 = wx.Panel(id=wxID_MAINFRAMEPANEL11, name='panel11',
              parent=self.circuitPanel, pos=wx.Point(0, 352), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel11.SetLabel(u'Inductor L')
        self.panel11.SetHelpText(u'')

        self.giV1 = wx.TextCtrl(id=wxID_MAINFRAMEGIV1, name=u'giV1',
              parent=self.panel11, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.giV1.Bind(wx.EVT_TEXT, self.OnGiV1Text, id=wxID_MAINFRAMEGIV1)

        self.staticText10 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT10,
              label=u'Initial V1', name='staticText10', parent=self.panel11,
              pos=wx.Point(8, 8), size=wx.Size(41, 13), style=0)

        self.panel12 = wx.Panel(id=wxID_MAINFRAMEPANEL12, name='panel12',
              parent=self.circuitPanel, pos=wx.Point(0, 392), size=wx.Size(312,
              40), style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel12.SetLabel(u'Inductor L')
        self.panel12.SetHelpText(u'')

        self.giV2 = wx.TextCtrl(id=wxID_MAINFRAMEGIV2, name=u'giV2',
              parent=self.panel12, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.giV2.Bind(wx.EVT_TEXT, self.OnGiV2Text, id=wxID_MAINFRAMEGIV2)

        self.staticText11 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT11,
              label=u'Initial V2', name='staticText11', parent=self.panel12,
              pos=wx.Point(8, 8), size=wx.Size(41, 13), style=0)

        self.panel13 = wx.Panel(id=wxID_MAINFRAMEPANEL13, name='panel13',
              parent=self.processingPanel, pos=wx.Point(0, 232),
              size=wx.Size(312, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel13.SetLabel(u'Inductor L')
        self.panel13.SetHelpText(u'')

        self.gkOutputGain = wx.TextCtrl(id=wxID_MAINFRAMEGKOUTPUTGAIN,
              name=u'gkOutputGain', parent=self.panel13, pos=wx.Point(152, 8),
              size=wx.Size(148, 21), style=0, value=u'1')
        self.gkOutputGain.Bind(wx.EVT_TEXT, self.OnOutputGainTextText,
              id=wxID_MAINFRAMEGKOUTPUTGAIN)

        self.staticText12 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT12,
              label=u'Output gain', name='staticText12', parent=self.panel13,
              pos=wx.Point(8, 8), size=wx.Size(57, 13), style=0)

        self.panel14 = wx.Panel(id=wxID_MAINFRAMEPANEL14, name='panel14',
              parent=self.processingPanel, pos=wx.Point(0, 32),
              size=wx.Size(312, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel14.SetLabel(u'Inductor L')
        self.panel14.SetHelpText(u'')

        self.gkstep_size = wx.TextCtrl(id=wxID_MAINFRAMEGKSTEP_SIZE,
              name=u'gkstep_size', parent=self.panel14, pos=wx.Point(152, 8),
              size=wx.Size(148, 21), style=0, value=u'')
        self.gkstep_size.Bind(wx.EVT_TEXT, self.OnGkstep_sizeText,
              id=wxID_MAINFRAMEGKSTEP_SIZE)

        self.staticText13 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT13,
              label=u'Time step', name='staticText13', parent=self.panel14,
              pos=wx.Point(8, 8), size=wx.Size(46, 13), style=0)

        self.panel15 = wx.Panel(id=wxID_MAINFRAMEPANEL15, name='panel15',
              parent=self.processingPanel, pos=wx.Point(0, 72),
              size=wx.Size(312, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel15.SetLabel(u'Inductor L')
        self.panel15.SetHelpText(u'')

        self.gkFilterFrequency = wx.TextCtrl(id=wxID_MAINFRAMEGKFILTERFREQUENCY,
              name=u'gkFilterFrequency', parent=self.panel15, pos=wx.Point(152,
              8), size=wx.Size(148, 21), style=0, value=u'16000')
        self.gkFilterFrequency.Bind(wx.EVT_TEXT, self.OnFilterFrequencyTextText,
              id=wxID_MAINFRAMEGKFILTERFREQUENCY)

        self.staticText14 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT14,
              label=u'Filter frequency', name='staticText14',
              parent=self.panel15, pos=wx.Point(8, 8), size=wx.Size(76, 13),
              style=0)

        self.panel16 = wx.Panel(id=wxID_MAINFRAMEPANEL16, name='panel16',
              parent=self.processingPanel, pos=wx.Point(0, 112),
              size=wx.Size(312, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel16.SetLabel(u'Inductor L')
        self.panel16.SetHelpText(u'')

        self.staticText15 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT15,
              label=u'Filter resonance', name='staticText15',
              parent=self.panel16, pos=wx.Point(8, 8), size=wx.Size(77, 13),
              style=0)

        self.panel17 = wx.Panel(id=wxID_MAINFRAMEPANEL17, name='panel17',
              parent=self.processingPanel, pos=wx.Point(0, 152),
              size=wx.Size(312, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel17.SetLabel(u'Inductor L')
        self.panel17.SetHelpText(u'')

        self.gkReverbDecay = wx.TextCtrl(id=wxID_MAINFRAMEGKREVERBDECAY,
              name=u'gkReverbDecay', parent=self.panel17, pos=wx.Point(152, 8),
              size=wx.Size(148, 21), style=0, value=u'.75')
        self.gkReverbDecay.Bind(wx.EVT_TEXT, self.OnReverberationDecayTextText,
              id=wxID_MAINFRAMEGKREVERBDECAY)

        self.staticText16 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT16,
              label=u'Reverberation decay', name='staticText16',
              parent=self.panel17, pos=wx.Point(8, 8), size=wx.Size(101, 13),
              style=0)

        self.panel18 = wx.Panel(id=wxID_MAINFRAMEPANEL18, name='panel18',
              parent=self.panel1, pos=wx.Point(320, 600), size=wx.Size(320,
              120), style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)

        self.staticText17 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT17,
              label=u'Performance', name='staticText17', parent=self.panel18,
              pos=wx.Point(8, 8), size=wx.Size(73, 13), style=0)
        self.staticText17.SetToolTipString(u'')
        self.staticText17.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD,
              False, u'MS Shell Dlg 2'))

        self.panel19 = wx.Panel(id=wxID_MAINFRAMEPANEL19, name='panel19',
              parent=self.panel18, pos=wx.Point(0, 32), size=wx.Size(312, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel19.SetLabel(u'Inductor L')
        self.panel19.SetHelpText(u'')

        self.staticText18 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT18,
              label=u'Audio output', name='staticText18', parent=self.panel19,
              pos=wx.Point(8, 8), size=wx.Size(62, 13), style=0)

        self.audioOutputText = wx.TextCtrl(id=wxID_MAINFRAMEAUDIOOUTPUTTEXT,
              name=u'audioOutputText', parent=self.panel19, pos=wx.Point(152,
              8), size=wx.Size(148, 21), style=0, value=u'dac')
        self.audioOutputText.Bind(wx.EVT_TEXT, self.OnAudioOutputTextText,
              id=wxID_MAINFRAMEAUDIOOUTPUTTEXT)

        self.playButton = wx.Button(id=wxID_MAINFRAMEPLAYBUTTON, label=u'Play',
              name=u'playButton', parent=self.panel18, pos=wx.Point(160, 80),
              size=wx.Size(72, 23), style=0)
        self.playButton.Bind(wx.EVT_BUTTON, self.OnPlayButtonButton,
              id=wxID_MAINFRAMEPLAYBUTTON)

        self.panel20 = wx.Panel(id=wxID_MAINFRAMEPANEL20, name='panel20',
              parent=self.panel1, pos=wx.Point(1, 161), size=wx.Size(640, 80),
              style=wx.RAISED_BORDER | wx.TAB_TRAVERSAL)

        self.panel21 = wx.Panel(id=wxID_MAINFRAMEPANEL21, name='panel21',
              parent=self.panel20, pos=wx.Point(0, 32), size=wx.Size(632, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel21.SetLabel(u'Inductor L')
        self.panel21.SetHelpText(u'')

        self.staticText19 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT19,
              label=u'Preset', name='staticText19', parent=self.panel21,
              pos=wx.Point(8, 8), size=wx.Size(31, 13), style=0)

        self.staticText20 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT20,
              label=u'Preset Circuits', name='staticText20',
              parent=self.panel20, pos=wx.Point(8, 8), size=wx.Size(82, 13),
              style=0)
        self.staticText20.SetToolTipString(u'')
        self.staticText20.SetFont(wx.Font(8, wx.SWISS, wx.NORMAL, wx.BOLD,
              False, u'MS Shell Dlg 2'))

        self.presetComboBox = wx.ComboBox(choices=[],
              id=wxID_MAINFRAMEPRESETCOMBOBOX, name=u'presetComboBox',
              parent=self.panel21, pos=wx.Point(72, 8), size=wx.Size(552, 21),
              style=0, value=u'')
        self.presetComboBox.SetLabel(u'')
        self.presetComboBox.Bind(wx.EVT_COMBOBOX, self.OnPresetComboBoxCombobox,
              id=wxID_MAINFRAMEPRESETCOMBOBOX)

        self.stopButton = wx.Button(id=wxID_MAINFRAMESTOPBUTTON, label=u'Stop',
              name=u'stopButton', parent=self.panel18, pos=wx.Point(232, 80),
              size=wx.Size(72, 23), style=0)
        self.stopButton.Bind(wx.EVT_BUTTON, self.OnStopButtonButton,
              id=wxID_MAINFRAMESTOPBUTTON)

        self.panel22 = wx.Panel(id=wxID_MAINFRAMEPANEL22, name='panel22',
              parent=self.processingPanel, pos=wx.Point(0, 192),
              size=wx.Size(312, 40),
              style=wx.RAISED_BORDER | wx.CAPTION | wx.TAB_TRAVERSAL)
        self.panel22.SetLabel(u'Inductor L')
        self.panel22.SetHelpText(u'')

        self.staticText21 = wx.StaticText(id=wxID_MAINFRAMESTATICTEXT21,
              label=u'Reverberation wet/dry mix', name='staticText21',
              parent=self.panel22, pos=wx.Point(8, 8), size=wx.Size(129, 13),
              style=0)

        self.gkReverbDryWetMix = wx.TextCtrl(id=wxID_MAINFRAMEGKREVERBDRYWETMIX,
              name=u'gkReverbDryWetMix', parent=self.panel22, pos=wx.Point(152,
              8), size=wx.Size(148, 21), style=0, value=u'.25')
        self.gkReverbDryWetMix.Bind(wx.EVT_TEXT,
              self.OnReverberationWetDryMixTextText,
              id=wxID_MAINFRAMEGKREVERBDRYWETMIX)

        self.gkFilterResonance = wx.TextCtrl(id=wxID_MAINFRAMEGKFILTERRESONANCE,
              name=u'gkFilterResonance', parent=self.panel16, pos=wx.Point(152,
              8), size=wx.Size(148, 21), style=0, value=u'0')
        self.gkFilterResonance.Bind(wx.EVT_TEXT, self.OnFilterResonanceTextText,
              id=wxID_MAINFRAMEGKFILTERRESONANCE)

        self.gkL = wx.TextCtrl(id=wxID_MAINFRAMEGKL, name=u'gkL',
              parent=self.panel3, pos=wx.Point(152, 8), size=wx.Size(148, 21),
              style=0, value=u'')
        self.gkL.Bind(wx.EVT_TEXT, self.OnGkLText, id=wxID_MAINFRAMEGKL)

    def __init__(self, parent):
        self._init_ctrls(parent)
        self.csound = csnd.CppSound()
        self.presets = {"periodic attractor + chaotic attractor (coexistence)": ['1048 ', '  .5 ', ' -1 ', ' -1 ', '  .136 ', '  .000929 ', '  100 ', ' -.699986 ', '  .599 ', '  .77 ', '  1 ', '  .28 ', '  0 ', ' -8.0294 ', '  .03163 ', ' -2.5348 ', '  25 ', '  2 ', '  .1 ', '  1 ', '  86 ', '  120 ', "periodic attractor + chaotic attractor (coexistence)"], "three chaotic attractors (coexistence)": ['1024 ', '  .05 ', '  0 ', ' -1 ', '  .0349895 ', '  0 ', '  1 ', '  1 ', ' -1.14285714 ', ' -.7142857 ', '  1 ', '  .06410256 ', '  0 ', ' -1.5407 ', '  .10764 ', '  .65153 ', '  1 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  40 ', "three chaotic attractors (coexistence)"], "periodic attractor ( bifurcation parameter is L )": ['1500 ', '  .012 ', '  0 ', ' -1 ', ' -4 ', '  2.228 ', ' -1.0837 ', '  33.932813 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  0 ', '  123.322875976563 ', ' -120.413566589356 ', ' -113.561897277832 ', '  .5 ', '  40 ', '  200 ', '  200 ', '  86 ', '  30 ', "periodic attractor ( bifurcation parameter is L )"], "chaotic attractor (intermittency route)": ['1500 ', '  .1 ', '  0 ', ' -1 ', '  .32 ', ' -.1 ', '  10 ', '  1 ', ' -.98 ', ' -2.4 ', '  1 ', ' -.1333 ', '  614.4 ', ' -.136550232768059 ', ' -1.46863320842385e-02 ', ' -1.01161253452301 ', '  7 ', '  1 ', '  .1 ', '  .4 ', '  66 ', '  131 ', "chaotic attractor (intermittency route)"], "strange attractor ( bifurcation parameter is  L )": ['1500 ', '  .012 ', '  0 ', ' -1 ', ' -50 ', '  2.228 ', ' -1.0837 ', '  33.932813 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  144 ', '  105.491371154785 ', ' -181.117050170898 ', ' -182.507720947266 ', '  .5 ', '  40 ', '  200 ', '  200 ', '  86 ', '  30 ', "strange attractor ( bifurcation parameter is  L )"], "First Created ABC file": ['1500', '0.03', ' 0', ' -1', '-1.45', '2.2280000000000002', '-1.0837000000000001', '33.333333333330003', '-0.5', '0.0064000000000000003', '1', '1', '315', '0.39119201898574801', '-0.75197964906692505', '-0.75856310129165705', '2', '0.2', '0.4', '0.4', '79', '88', "First Created ABC file"], "two chaotic attractors (coexistence)": ['1024 ', '  .5 ', '  0 ', ' -1 ', '  .35 ', '  0 ', '  10 ', '  1 ', ' -1.143 ', ' -.7143 ', '  1 ', '  .641 ', '  0 ', ' -1.511 ', ' -.0795 ', '  1.15 ', '  25 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  40 ', "two chaotic attractors (coexistence)"], "period-2:2 attractor (period adding)": ['1024 ', '  .02 ', ' -1 ', ' -1 ', '  .12 ', '  .03086 ', '  .2132 ', '  .6839945 ', ' -.879 ', ' -.4124 ', '  1 ', '  .0575 ', '  40.96 ', '  1.12937533855438 ', ' -.634763240814209 ', ' -2.43482446670532 ', '  1 ', '  1 ', '  .2 ', '  1 ', '  86 ', '  30 ', "period-2:2 attractor (period adding)"], "period-4:4 attractor (period adding)": ['1024 ', '  .02 ', ' -1 ', ' -1 ', '  .12 ', '  .03086 ', '  .2132 ', '  .667557 ', ' -.879 ', ' -.4124 ', '  1 ', '  .0575 ', '  40.96 ', '  1.44595527648926 ', '  .353626161813736 ', ' -1.59986960887909 ', '  1 ', '  1 ', '  .2 ', '  1 ', '  86 ', '  30 ', "period-4:4 attractor (period adding)"], "point attractor + periodic attractor  + chaotic attractor (coexistence)": ['5000', '5e-006', ' 0', ' -1', '0.017999999999999999', '12.5', '9.9600000000000005e-008', '0.00060643000000000003', '-0.00076057999999999996', '-0.00040800999999999999', '1', '11.1e-9', '0', '0', '-0.10000000000000001', '0.100000000002', '2.5', '20', '20', '10', '89', '0', "point attractor + periodic attractor  + chaotic attractor (coexistence)"], "strange attractor ( bifurcation parameter is R0 )": ['1500 ', '  .04 ', '  0 ', ' -1 ', ' -9.7136 ', '  4.9 ', ' -1.0837 ', '  33.932813 ', ' -.5 ', '  .0064 ', '  1 ', '  1 ', '  240 ', '  .76497608423233 ', ' -3.262779712677 ', ' -3.19569420814514 ', '  2.5 ', '  .2 ', '  2 ', '  2 ', '  356 ', '  30 ', "strange attractor ( bifurcation parameter is R0 )"], "periodic attractor ( gallery of attractors )": ['1024 ', '  1 ', ' -1 ', ' -1 ', ' -5 ', '  0 ', '  10 ', '  1 ', ' -1.14292 ', ' -.7142 ', '  1 ', ' -2.45 ', '  1024 ', ' -1.24378001689911 ', '  .753904342651367 ', '  2.66716170310974 ', '  50 ', '  .4 ', '  .2 ', '  1 ', '  76 ', '  310 ', "periodic attractor ( gallery of attractors )"], "period-1 attractor ( bifurcation parameter is R0 )": ['1500 ', '  .04 ', '  0 ', ' -1 ', ' -9.7136 ', '  4.8 ', ' -1.0837 ', '  33.932813 ', ' -.5 ', '  .0064 ', '  1 ', '  1 ', '  300 ', '  .449593305587769 ', ' -4.77259922027588 ', ' -4.76103210449219 ', '  2.5 ', '  .2 ', '  2 ', '  2 ', '  356 ', '  30 ', "period-1 attractor ( bifurcation parameter is R0 )"], "heteroclinic orbit": ['1500 ', '  .425 ', '  0 ', ' -1 ', '  1.3506168 ', '  0 ', ' -4.50746268737 ', ' -1 ', '  2.4924 ', '  .93 ', '  1 ', '  1 ', '  0 ', ' -22.28662665 ', '  .009506608 ', ' -22.2861576 ', '  32 ', '  10 ', '  2 ', '  20 ', '  86 ', '  30 ', "heteroclinic orbit"], "chaotic attractor (torus breakdown route)": ['1500 ', '  .02 ', ' -1 ', ' -1 ', '  .00667 ', '  .000651 ', '  10 ', ' -1 ', '  .856 ', '  1.1 ', '  1 ', '  .0294 ', '  80.48 ', ' -11.4031324386597 ', '  .142968431115151 ', ' -2.16099524497986 ', '  1 ', '  10 ', '  .2 ', '  1 ', '  66 ', '  81 ', "chaotic attractor (torus breakdown route)"], "two asymmetric periodic attractors (symmetry breaking)": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .11299021 ', '  .023663 ', '  .71429 ', '  1 ', ' -1.06060606 ', ' -.7511967 ', '  1 ', '  .1233684 ', '  51.2 ', '  .291747 ', ' -.023522 ', ' -.543712 ', '  2.5 ', '  1 ', '  .1 ', '  .5 ', '  86 ', '  40 ', "two asymmetric periodic attractors (symmetry breaking)"], "torus attractor (torus breakdown route)": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .00667 ', '  .000651 ', '  10 ', ' -1 ', '  .856 ', '  1.1 ', '  1 ', '  .051 ', '  51.2 ', ' -3.33239960670471 ', ' -.288763225078583 ', ' -1.23194742202759 ', '  2.5 ', '  10 ', '  .2 ', '  1 ', '  66 ', '  81 ', "torus attractor (torus breakdown route)"], "homoclinic orbit": ['1500 ', '  .14 ', '  0 ', ' -1 ', '  .463659975 ', '  0 ', '  50 ', '  1 ', ' -1.1428571 ', ' -.71428571 ', '  1 ', '  1 ', '  0 ', ' -.000463001 ', '  .0000405017 ', '  .000885432 ', '  10 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  30 ', "homoclinic orbit"], "two asymmetric periodic attractors (symmetry breaking)": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .11299021 ', '  .023663 ', '  .71429 ', '  1 ', ' -1.06060606 ', ' -.7511967 ', '  1 ', '  .1233684 ', '  51.2 ', ' -.291747123003006 ', '  2.35222522169352e-02 ', '  .543711602687836 ', '  2.5 ', '  1 ', '  .1 ', '  .5 ', '  86 ', '  40 ', "two asymmetric periodic attractors (symmetry breaking)"], "period-1 attractor ( bifurcation parameter is R )": ['1500 ', '  .004 ', '  0 ', ' -1 ', ' -12 ', '  2.228 ', ' -1.0837 ', '  15.384615 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  36 ', ' -83.8893280029297 ', '  9.95018863677979 ', '  30.5339050292969 ', '  .2 ', '  40 ', '  100 ', '  100 ', '  356 ', '  30 ', "period-1 attractor ( bifurcation parameter is R )"], "odd-symmetric periodic attractor (symmetry breaking)": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .11299021 ', '  .023663 ', '  .83333 ', '  1 ', ' -1.06060606 ', ' -.7511967 ', '  1 ', '  .1233684 ', '  153.6 ', '  1.98223829269409 ', ' -.21183779835701 ', ' -1.94790291786194 ', '  2.5 ', '  1 ', '  .1 ', '  .5 ', '  86 ', '  40 ', "odd-symmetric periodic attractor (symmetry breaking)"], "periodic attractor (torus breakdown route)": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .00667 ', '  .000651 ', '  10 ', ' -1 ', '  .856 ', '  1.1 ', '  1 ', '  .035 ', '  204.8 ', ' -10.01 ', '  .18698 ', ' -1.664 ', '  2.5 ', '  10 ', '  .2 ', '  1 ', '  66 ', '  81 ', "periodic attractor (torus breakdown route)"], "period-2 attractor ( bifurcation parameter is R0 )": ['1500 ', '  .04 ', '  0 ', ' -1 ', ' -9.7136 ', '  4.75 ', ' -1.0837 ', '  33.932813 ', ' -.5 ', '  .0064 ', '  1 ', '  1 ', '  300 ', '  .722549259662628 ', ' -4.87812995910645 ', ' -4.80317687988281 ', '  2.5 ', '  .2 ', '  2 ', '  2 ', '  356 ', '  30 ', "period-2 attractor ( bifurcation parameter is R0 )"], "period-8 attractor (period-doubling route)": ['512 ', '  .05 ', ' -1 ', ' -1 ', '  .0625 ', '  0 ', '  1 ', '  1 ', ' -1.143 ', ' -.714 ', '  1 ', '  .10915 ', '  76.8 ', '  1.9290052652359 ', '  .347841590642929 ', ' -1.18657028675079 ', '  1 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  51 ', "period-8 attractor (period-doubling route)"], "period-4 attractor (period-doubling route)": ['512 ', '  .05 ', ' -1 ', ' -1 ', '  .0625 ', '  0 ', '  1 ', '  1 ', ' -1.143 ', ' -.714 ', '  1 ', '  .10965 ', '  102.4 ', ' -.6163689494133 ', '  .112614527344704 ', ' -.14087986946106 ', '  1 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  51 ', "period-4 attractor (period-doubling route)"], "periodic attractor (intermittency route)": ['1500 ', '  .05 ', '  0 ', ' -1 ', '  .2232 ', ' -.1 ', '  10 ', '  1 ', ' -.98 ', ' -2.4 ', '  1 ', ' -.1333 ', '  102.4 ', '  .509903907775879 ', '  3.11504732817411e-02 ', '  1.00217819213867 ', '  3 ', '  1 ', '  .1 ', '  .4 ', '  66 ', '  131 ', "periodic attractor (intermittency route)"], "transient behavior (preturbulence)": ['1500 ', '  .004 ', '  0 ', ' -1 ', ' -.0132099 ', ' -.01589636 ', ' -1 ', '  1 ', ' -.855372 ', ' -1.09956 ', '  1 ', ' -.02782478 ', '  36 ', '  1.54400110244751 ', ' -.395819276571274 ', ' -1.79381775856018 ', '  .25 ', '  2 ', '  .2 ', '  2 ', '  76 ', '  40 ', "transient behavior (preturbulence)"], "point attractor + periodic attractor + chaotic attractor (coexistence)": ['1024 ', '  .05 ', '  0 ', ' -1 ', '  .142857 ', '  0 ', '  1 ', '  .7 ', ' -.8 ', ' -.1 ', '  1 ', '  .10526 ', '  102.4 ', ' -.06 ', '  .15 ', ' -.29 ', '  2.5 ', '  1 ', '  .2 ', '  .5 ', '  86 ', '  40 ', "point attractor + periodic attractor + chaotic attractor (coexistence)"], "point attractor + periodic attractor  + chaotic attractor (coexistence)": ['1024 ', '  .05 ', '  0 ', ' -1 ', '  .142857 ', '  0 ', '  1 ', '  .7 ', ' -.8 ', ' -.1 ', '  1 ', '  .10526 ', '  51.2 ', ' -120.245 ', '  8.60401 ', '  27.9499 ', '  2.5 ', '  20 ', '  20 ', '  10 ', '  86 ', '  40 ', "point attractor + periodic attractor  + chaotic attractor (coexistence)"], "two point attractors + three periodic attractors (coexistence)": ['1024 ', '  .02 ', ' -1 ', ' -1 ', '  .142857 ', '  0 ', '  1 ', '  .7 ', ' -.8 ', ' -.1 ', '  1 ', '  .125 ', '  0 ', '  1.4002 ', '  0 ', ' -1.2612 ', '  1 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  40 ', "two point attractors + three periodic attractors (coexistence)"], "strange attractor ( bifurcation parameter is C2 )": ['1500 ', '  .012 ', '  0 ', ' -1 ', ' -12 ', '  2.228 ', ' -1.3 ', '  18.867925 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  558 ', ' -2.72707533836365 ', ' -26.4243125915527 ', ' -16.8644371032715 ', '  .5 ', '  40 ', '  200 ', '  200 ', '  116 ', '  30 ', "strange attractor ( bifurcation parameter is C2 )"], "period-5:5 attractor (period adding)": ['1024 ', '  .02 ', ' -1 ', ' -1 ', '  .12 ', '  .03086 ', '  .2132 ', '  .662252 ', ' -.879 ', ' -.4124 ', '  1 ', '  .0575 ', '  40.96 ', '  1.0731498003006 ', ' -.678816974163055 ', ' -2.51866912841797 ', '  1 ', '  1 ', '  .2 ', '  1 ', '  86 ', '  30 ', "period-5:5 attractor (period adding)"], "period-2 attractor ( bifurcation parameter is R )": ['1500 ', '  .002 ', ' -1 ', ' -1 ', ' -12 ', '  2.228 ', ' -1.0837 ', '  16.6666666666667 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  15 ', ' -85.6029663085938 ', '  72.0791168212891 ', '  88.9068145751953 ', '  .1 ', '  40 ', '  100 ', '  100 ', '  356 ', '  30 ', "period-2 attractor ( bifurcation parameter is R )"], "period-2 attractor (period-doubling route)": ['512 ', '  .05 ', ' -1 ', ' -1 ', '  .0625 ', '  0 ', '  1 ', '  1 ', ' -1.143 ', ' -.714 ', '  1 ', '  .1105 ', '  25.6 ', '  2.8466784954071 ', ' -.068253830075264 ', ' -1.9940402507782 ', '  1 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  51 ', "period-2 attractor (period-doubling route)"], "chaotic attractor ( gallery of attractors )": ['1500 ', '  .002 ', '  0 ', ' -1 ', '  .106 ', '  3.43 ', ' -.01 ', '  1 ', '  1.219 ', ' -.514 ', '  1 ', '  .00684 ', '  4.096 ', '  .649512827396393 ', ' -1.25815808773041 ', ' -.429547637701035 ', '  .1 ', '  .4 ', '  2 ', '  4 ', '  116 ', '  90 ', "chaotic attractor ( gallery of attractors )"], "Spiral Chuas attractor (period-doubling route)": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .0625 ', '  0 ', '  1 ', '  1 ', ' -1.143 ', ' -.714 ', '  1 ', '  .10753 ', '  51.2 ', '  2.70433306694031 ', '  .234987959265709 ', ' -1.58762669563294 ', '  2.5 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  51 ', "Spiral Chuas attractor (period-doubling route)"], "period-8 attractor ( bifurcation parameter is R )": ['1500 ', '  .002 ', ' -1 ', ' -1 ', ' -12 ', '  2.228 ', ' -1.0837 ', '  17.123288 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  51 ', ' -84.3716888427734 ', '  138.481033325195 ', '  153.974487304688 ', '  .1 ', '  40 ', '  100 ', '  100 ', '  356 ', '  30 ', "period-8 attractor ( bifurcation parameter is R )"], "strange attractor ( bifurcation parameter is R )": ['1500 ', '  .002 ', ' -1 ', ' -1 ', ' -12 ', '  2.228 ', ' -1.0837 ', '  17.152659 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  57 ', ' -88.8154144287109 ', '  262.036071777344 ', '  262.645782470703 ', '  .1 ', '  40 ', '  100 ', '  100 ', '  356 ', '  30 ', "strange attractor ( bifurcation parameter is R )"], "torus attractor ( gallery of attractors )": ['2048 ', '  .02 ', ' -1 ', ' -1 ', '  .01044701 ', ' -.00938375 ', '  1 ', '  1 ', ' -.855372 ', ' -1.09956 ', '  1 ', '  .08236273 ', '  40.96 ', ' -1.41563788056374e-02 ', ' -1.55622684955597 ', '  .234290078282356 ', '  1 ', '  10 ', '  .4 ', '  1 ', '  66 ', '  30 ', "torus attractor ( gallery of attractors )"], "period-6:6 attractor (period adding)": ['1024 ', '  .02 ', ' -1 ', ' -1 ', '  .12 ', '  .03086 ', '  .2132 ', '  .655308 ', ' -.879 ', ' -.4124 ', '  1 ', '  .0575 ', '  20.48 ', ' -1.45486557483673 ', '  .549690783023834 ', '  2.72495412826538 ', '  1 ', '  1 ', '  .2 ', '  1 ', '  86 ', '  30 ', "period-6:6 attractor (period adding)"], "period-4 attractor ( bifurcation parameter is R )": ['1500 ', '  .002 ', ' -1 ', ' -1 ', ' -12 ', '  2.228 ', ' -1.0837 ', '  17.094017 ', ' -100 ', ' -.003 ', '  1 ', '  1 ', '  51 ', ' -84.63134765625 ', '  130.960418701172 ', '  146.48957824707 ', '  .1 ', '  40 ', '  100 ', '  100 ', '  356 ', '  30 ', "period-4 attractor ( bifurcation parameter is R )"], "period-3:3 attractor (period adding)": ['1024 ', '  .02 ', ' -1 ', ' -1 ', '  .12 ', '  .03086 ', '  .2132 ', '  .674764 ', ' -.879 ', ' -.4124 ', '  1 ', '  .0575 ', '  20.48 ', ' -.34927961230278 ', '  9.31535810232163e-02 ', '  1.32918405532837 ', '  1 ', '  1 ', '  .2 ', '  1 ', '  86 ', '  30 ', "period-3:3 attractor (period adding)"], "periodic attractor after transient has died out (preturbulence)": ['1500 ', '  .004 ', '  0 ', ' -1 ', ' -.0132099 ', ' -.01589636 ', ' -1 ', '  1 ', ' -.855372 ', ' -1.09956 ', '  1 ', ' -.02782478 ', '  0 ', ' -5.59 ', '  .01685 ', '  3.6 ', '  .25 ', '  2 ', '  .2 ', '  2 ', '  76 ', '  40 ', "periodic attractor after transient has died out (preturbulence)"], "semiperiodic behavior": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .11299021 ', '  .023663 ', '  1 ', '  .9337068 ', ' -1.06060606 ', ' -.7511967 ', '  1 ', '  .1233684 ', '  562.65 ', '  1.71052622795105 ', ' -.213898345828056 ', ' -2.10661435127258 ', '  1 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  40 ', "semiperiodic behavior"], "Double Scroll Chuas attractor (period-doubling route)": ['1500 ', '  .05 ', ' -1 ', ' -1 ', '  .0625 ', '  0 ', '  1 ', '  1 ', ' -1.143 ', ' -.714 ', '  1 ', '  .10204 ', '  0 ', ' -.656194627285004 ', '  .250746101140976 ', '  1.51263117790222 ', '  2.5 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  51 ', "Double Scroll Chuas attractor (period-doubling route)"], "chaotic attractor ( gallery of attractors )": ['1024', '1', ' 0', ' -1', '-4722.8099', '0', '0.010587', '0.0010587', '-1.1429', '-0.7142', '1', '-0.0025726', '5120', '-0.87001651525497403', '0.084834069013595595', '0.89790028333663896', '50', '0.4', '0.2', '1', '246', '0', '0.000176', '0', '-0.00121', '0', "chaotic attractor ( gallery of attractors )"], "period-3 attractor ( bifurcation parameter is R0 )": ['1500 ', '  .04 ', '  0 ', ' -1 ', ' -9.7136 ', '  4.7 ', ' -1.0837 ', '  33.932813 ', ' -.5 ', '  .0064 ', '  1 ', '  1 ', '  1860 ', '  .752286672592163 ', ' -3.93914341926575 ', ' -3.8665657043457 ', '  2.5 ', '  .2 ', '  2 ', '  2 ', '  356 ', '  30 ', "period-3 attractor ( bifurcation parameter is R0 )"], "period-1 attractor (period-doubling route)": ['512 ', '  .05 ', ' -1 ', ' -1 ', '  .0625 ', '  0 ', '  1 ', '  1 ', ' -1.143 ', ' -.714 ', '  1 ', '  .11364 ', '  76.8 ', '  3.10581803321838 ', ' -2.94936783611774e-02 ', ' -2.04718446731567 ', '  1 ', '  1 ', '  .1 ', '  1 ', '  76 ', '  51 ', "period-1 attractor (period-doubling route)"], "chaotic attractor ( gallery of attractors )": ['1024 ', '  .1 ', ' -1 ', ' -1 ', '  .047 ', '  .0041 ', '  10 ', ' -1 ', ' -.474 ', '  2.039 ', '  1 ', '  .751 ', '  102.4 ', ' -16.4638290405273 ', '  2.33884620666504 ', '  .441048443317413 ', '  5 ', '  10 ', '  1 ', '  1 ', '  86 ', '  30 ', "chaotic attractor ( gallery of attractors )"], "odd-symmetric periodic attractor (symmetry breaking)": ['1024 ', '  .05 ', ' -1 ', ' -1 ', '  .11299021 ', '  .023663 ', '  .625 ', '  1 ', ' -1.06060606 ', ' -.7511967 ', '  1 ', '  .1233684 ', '  153.6 ', ' -1.7799619436264 ', ' -.189503252506256 ', '  1.31620240211487 ', '  2.5 ', '  1 ', '  .1 ', '  .5 ', '  86 ', '  40 ', "odd-symmetric periodic attractor (symmetry breaking)"], "Chuas circuit with Cubic Nonlinearity": ['1000', '0.000005', ' 0', ' -1', '0.0094500000000000001', '7.5', '1.9999999999999999e-007', '0.0010499999999999999', '0', '-0.0012099999999999999', '1.7600000000000001e-005', '1.4999999999999999e-008', '0', '0', '-0.10000000000000001', '0.10000000000000001', '-0.1', '20', '20', '10', '90', '0', '1.7600000000000001e-005', '0', '-0.0012099999999999999', '0', "Chuas circuit with Cubic Nonlinearity"]}
        for key in self.presets.keys():
            self.presetComboBox.Append(key)

    def playThreadRoutine(self):
        try:
            print 'playThreadRoutine...'
            self.csound.setOrchestra(orchestra)
            self.csound.setScore(score)
            self.csound.setCommand('csound -h -m7 -d -o%s -B400 -b400 temp.orc temp.sco' % str((self.audioOutputText.GetValue())))
            self.csound.exportForPerformance()
            self.csound.compile()
            self.csound.SetChannel('gkL', float(self.gkL.GetValue()))
            self.csound.SetChannel('giI3', float(self.giI3.GetValue()))
            self.csound.SetChannel('gkR0', float(self.gkR0.GetValue()))
            self.csound.SetChannel('gkC1', float(self.gkC1.GetValue()))
            self.csound.SetChannel('gkC2', float(self.gkC2.GetValue()))
            self.csound.SetChannel('gkG', float(self.gkG.GetValue()))
            self.csound.SetChannel('gkGa', float(self.gkGa.GetValue()))
            self.csound.SetChannel('gkGb', float(self.gkGb.GetValue()))
            self.csound.SetChannel('gkE', float(self.gkE.GetValue()))
            self.csound.SetChannel('giV1', float(self.giV1.GetValue()))
            self.csound.SetChannel('giV2', float(self.giV2.GetValue()))
            self.csound.SetChannel('gkOutputGain', float(self.gkOutputGain.GetValue()))
            self.csound.SetChannel('gkstep_size', float(self.gkstep_size.GetValue()))
            self.csound.SetChannel('gkFilterFrequency', float(self.gkFilterFrequency.GetValue()))
            self.csound.SetChannel('gkReverbDecay', float(self.gkReverbDecay.GetValue()))
            self.csound.SetChannel('gkReverbDryWetMix', float(self.gkReverbDryWetMix.GetValue()))
            self.csound.SetChannel('gkFilterResonance', float(self.gkFilterResonance.GetValue()))
            self.csoundPerformanceThread = csnd.CsoundPerformanceThread(self.csound)
            self.csoundPerformanceThread.Play()
            print 'Playing...'
        except:
            traceback.print_exc()
            
    def OnPlayButtonButton(self, event):
        print 'OnPlayButtonButton...'
        self.csoundThread = threading.Thread(None, self.playThreadRoutine)
        self.csoundThread.start()
 
    def OnStopButtonButton(self, event):
        print 'OnStopButtonButton...'
        try:
            print 'Ending performance...'
            self.csoundPerformanceThread.Stop()
            self.csoundPerformanceThread.Join()
            self.csoundThread.join()
            print 'Csound thread has finished.'
        except:
            print traceback.print_exc()
            

    def OnPresetComboBoxCombobox(self, event):
        self.preset = self.presets[self.presetComboBox.GetValue()]
        print "Preset:", self.preset
        # sys_variables = system_vars(5:12); % L,R0,C2,G,Ga,Gb,E,C1 or p8:p15
        # integ_variables = [system_vars(14:16),system_vars(1:2)]; % x0,y0,z0,dataset_size,step_size or p17:p19, p4:p5
        self.gkstep_size.SetValue(str(float(self.preset[1])))
        self.gkL.SetValue(str(float(self.preset[4])))
        self.gkR0.SetValue(str(float(self.preset[5])))
        self.gkC2.SetValue(str(float(self.preset[6])))
        self.gkG.SetValue(str(float(self.preset[7])))
        self.gkGa.SetValue(str(float(self.preset[8])))
        self.gkGb.SetValue(str(float(self.preset[9])))
        self.gkE.SetValue(str(float(self.preset[10])))
        self.gkC1.SetValue(str(float(self.preset[11])))
        self.giI3.SetValue(str(float(self.preset[13])))
        self.giV2.SetValue(str(float(self.preset[14])))
        self.giV1.SetValue(str(float(self.preset[15])))
    
    def setValue(self, event):
        objekt = event.GetEventObject()
        channel = str(objekt.GetName())
        value = float(objekt.GetValue())
        print '%12s: %f' % (channel, value)
        self.csound.SetChannel(channel, value)

    def OnGkLText(self, event):
        self.setValue(event)
 
    def OnGiI3Text(self, event):
        self.setValue(event)

    def OnGkR0Text(self, event):
        self.setValue(event)

    def OnGkC1Text(self, event):
        self.setValue(event)

    def OnGkC2Text(self, event):
        self.setValue(event)

    def OnGkGText(self, event):
        self.setValue(event)

    def OnGkGaText(self, event):
        self.setValue(event)

    def OnGkGbText(self, event):
        self.setValue(event)

    def OnGkEText(self, event):
        self.setValue(event)

    def OnGiV1Text(self, event):
        self.setValue(event)

    def OnGiV2Text(self, event):
        self.setValue(event)

    def OnOutputGainTextText(self, event):
        value = float(event.GetEventObject().GetValue())
        print "gkOutputGain:", value
        self.csound.SetChannel('gkOutputGain', value)

    def OnGkstep_sizeText(self, event):
        self.setValue(event)

    def OnFilterFrequencyTextText(self, event):
        self.setValue(event)

    def OnReverberationDecayTextText(self, event):
        self.setValue(event)

    def OnAudioOutputTextText(self, event):
        value = event.GetEventObject().GetValue()
        print "Audio output:", value

    def OnReverberationWetDryMixTextText(self, event):
        value = float(event.GetEventObject().GetValue())
        print "Value:", value
        self.csound.SetChannel('gkReverbDryWetMix', value)

    def OnFilterResonanceTextText(self, event):
        value = float(event.GetEventObject().GetValue())
        print "Value:", value
        self.csound.SetChannel('gkFilterResonance', value)
        

orchestra = '''
sr      = 44100
ksmps   = 100
nchnls  = 2
0dbfs   = 1000

; Set up global control channels for use by Csound API.
gkstep_size         init            1.0
gkstep_size         chnexport       "gkstep_size", 3
gkL                 init            1.0
gkL                 chnexport       "gkL", 3
gkR0                init            1.0
gkR0                chnexport       "gkR0", 3
gkC2                init            1.0
gkC2                chnexport       "gkC2", 3
gkG                 init            1.0
gkG                 chnexport       "gkG", 3
gkGa                init            1.0
gkGa                chnexport       "gkGa", 3
gkGb                init            1.0
gkGb                chnexport       "gkGb", 3
gkE                 init            1.0
gkE                 chnexport       "gkE", 3
gkC1                init            1.0
gkC1                chnexport       "gkC1", 3
giV1                init            1.0
giV1                chnexport       "giV1", 3
giV2                init            1.0
giV2                chnexport       "giV2", 3
giI3                init            1.0
giI3                chnexport       "giI3", 3
gkFilterFrequency   init            1.0
gkFilterFrequency   chnexport       "gkFilterFrequency", 3
gkFilterResonance   init            1.0
gkFilterResonance   chnexport       "gkFilterResonance", 3
gkReverbDelay       init            1.0
gkReverbDelay       chnexport       "gkReverbDelay", 3
gkReverbWetDryMix   init            1.0
gkReverbWetDryMix   chnexport       "gkReverbWetDryMix", 3
gkOutputGain        init            1.0
gkOutputGain        chnexport       "gkOutputGain", 3

gibuzztable         ftgen           1, 0, 16384, 10, 1

                    instr  1
                    ; sys_variables = system_vars(5:12); % L,R0,C2,G,Ga,Gb,E,C1 or p8:p15
                    ; integ_variables = [system_vars(14:16),system_vars(1:2)]; % x0,y0,z0,dataset_size,step_size or p17:p19, p4:p5
iattack             =               0.02
isustain            =               p3
irelease            =               0.02
p3                  =               iattack + isustain + irelease
iscale              =               1.0
adamping            linsegr         0.0, iattack, iscale, isustain, iscale, irelease, 0.0
aguide              buzz            100.0, 880, sr/880, gibuzztable
                    ; printk          1.0, gkL
                    ; Chua's oscillator with piecewise nonlinearity, circuit elements controlled by GUI.
aI3, aV2, aV1       chuap           gkL, gkR0, gkC2, gkG, gkGa, gkGb, gkE, gkC1, giI3, giV2, giV1, gkstep_size	
                    ; Try to normalize volume.
abalanced           balance         aV2, aguide
                    ; Resonant filter.
afiltered           moogladder      abalanced, gkFilterFrequency, gkFilterResonance, 1.00
                    ; Reverberation
arleft, arright     reverbsc        afiltered, afiltered, gkReverbDelay, 16000 
arwetleft           =               gkReverbWetDryMix * arleft
arwetright          =               gkReverbWetDryMix * arright
ardry               =               (1.0 - gkReverbWetDryMix) * afiltered
                    ; Anti-clicking.
adamping            linsegr         0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
                    outs            adamping * (arwetleft + ardry) * gkOutputGain, adamping * (arwetright + ardry) * gkOutputGain
                    endin
'''

score = '''
; An endless note.
f 0 6000
i 1 1 -1
'''
