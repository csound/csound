#!/usr/bin/env python
#Boa:App:BoaApp

import wx

import ChuasOscillatorFrame

modules ={'ChuasOscillatorFrame': [1, 'Main frame of Application', 'ChuasOscillatorFrame.py'],
 'CsoundPlayer': [0, '', 'CsoundPlayer.py']}

class BoaApp(wx.App):
    def OnInit(self):
        wx.InitAllImageHandlers()
        self.main = ChuasOscillatorFrame.create(None)
        self.main.Show()
        self.SetTopWindow(self.main)
        return True

def main():
    application = BoaApp(0)
    application.MainLoop()

if __name__ == '__main__':
    main()
