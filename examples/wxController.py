# my demo app of a button and slider that sends events to some functions 
# this one uses the style from the wxPython wiki
# I would like to embed csound so that the functions can instead make api calls to csound


# import wx
from wxPython.wx import *

# Automatically creates global csound object.

import CsoundVST 

ID_BUTTON1 = 10
ID_SLIDER1 = 20

# Csound MUST run in its own thread

import threading

def csoundThreadRoutine():
    # Embed an orchestra in this script.
    csound.setOrchestra('''
    sr=44100
    kr=441
    ksmps=100
    nchnls=2

        instr	1
        print p1, p2, p3, p4, p5
        ; Slider sends note with MIDI key number.
        ; Convert it to octaves.
        ioctave = p4 / 12.0 + 3.0
        print ioctave
        ; Convert octave to frequency.
        ihertz = cpsoct(ioctave)
    a1	oscili	ampdb(p5), ihertz, 1
        outs	a1,a1
        endin
    ''')
    # And a score.
    csound.setScore('''
    f1 0 8192 10 1
    f0 60
    i 1 0 10 60 60
    e
    ''')

    # Real-time audio output, don't forget to enable line events.
    
    csound.setCommand('''csound -d -Lstdin -odac6 temp.orc temp.sco''')

    # Export the orc and sco.

    csound.exportForPerformance()

    # Start the performance.

    csound.compile()
    while(True):
        csound.performKsmps()
        
        # Absolutely MUST yield to wxWindows or the app will freeze!
        
        wx.wxYield()

# Create the Csound thread, but don't start it just yet.

csoundThread = threading.Thread(None, csoundThreadRoutine)
csoundThread.start()

# declare my ControlPanel class, inherit from standard wx.Panel
class ControlPanel( wxPanel ):

	# overide the standard wx.Panel constructor	
	def __init__(self, parent):

		# call the parent classes constructor to begin with	
		wxPanel.__init__( self, parent, -1 )	
		
		# add a button, event is ID_BUTTON1		
		button1 = wxButton(self, ID_BUTTON1, "Send Note", (20, 20))
		EVT_BUTTON(self, ID_BUTTON1, self.OnClickButton1 )
		
		# below is method that works in windows but not on my linux version, weird
		# on linux I get a no method called Bind message
		# self.Bind(wx.EVT_BUTTON, self.OnClickButton1, button1 )
	
	
		# add a slider that sends an event on any change
		slider1 = wxSlider(self, ID_SLIDER1, 63, 0, 127, (20, 50), (200,50),
				   wxSL_HORIZONTAL | wxSL_LABELS )
        	slider1.SetTickFreq(5, 1)
		EVT_SLIDER(self, ID_SLIDER1, self.OnSlider1Move )

	# methods of the ControlPanel class
	def OnClickButton1(self, event):
		# call my external function
		# send it the current value of the slider
		button1( self.slider1_val )

	def OnSlider1Move(self, event):
		slider1()
		self.slider1_val = event.GetInt()

	# the slider value data member of the ControlPanel
	slider1_val = 0


#--------------------------------------------------------------------------------
# Functions outside of the control class
# these would be functions to interface with the csound API

def button1( value ):
	print "You pressed button 1"
	print "The value sent is: %i" % value
	# Send a line event.
	csound.inputMessage("i 1 0 1 %i 70" % value)

def slider1():
	print "You moved slider 1"
	
	

#--------------------------------------------------------------------------------
# Main body of app

ctrl_app = wxPySimpleApp()

# start_pos = ( 400,20 )
# start_size = ( 400,300 )
# frame = wxFrame( None, -1, "Csound Controller", start_pos, start_size )
frame = wxFrame( None, -1, "Csound Controller" )
ctrl_panel = ControlPanel( frame )
frame.Show(True)
ctrl_app.MainLoop()

# End of main body



