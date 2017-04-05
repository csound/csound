# Importing CsoundAC automatically creates a global 'csound' object.
import csnd6
import wx
# Csound MUST run in its own thread.
import threading
import traceback

# Create a panel to hold widgets that will control a Csound performance.
class ControlPanel(wx.Panel):
        # Override the base class constructor.
        def __init__(self, parent):
            wx.Panel.__init__(self, parent, -1)
            # Create a button to send a note to Csound.
            self.ID_BUTTON1 = 10
            self.button1 = wx.Button(self, self.ID_BUTTON1, "Send Note", (20, 20))
            # Bind the button to its event handler.
            wx.EVT_BUTTON(self, self.ID_BUTTON1, self.OnClickButton1)
            # Create a slider to change the pitch of the note.
            self.ID_SLIDER1 = 20
            self.slider1 = wx.Slider(self, self.ID_SLIDER1, 63, 0, 127, (20, 50), (200,50),
                                    wx.SL_HORIZONTAL | wx.SL_LABELS)
            self.slider1.SetTickFreq(5, 1)
            # Bind the slider to its event handler.
            wx.EVT_SLIDER(self, self.ID_SLIDER1, self.OnSlider1Move)
            # Set up a 'close' event handler to cleanly shut down Csound.
            wx.EVT_CLOSE(parent, self.OnClose)
            # Default pitch.
            self.pitch = 60
            # Create an instance of CppSound.
            self.csound = csnd6.CppSound()
            # Create the Csound thread and start it.
            self.csoundThread = threading.Thread(None, self.csoundThreadRoutine)
            self.csoundThread.start()

        # Initialize Csound and start performing, in a separate thread.
        # Csound will play notes sent in from wxWindows.
        def csoundThreadRoutine(self):
            # Embed an orchestra in this script.
            self.csound.setOrchestra('''
            sr=44100
            ksmps=128
            nchnls=2
                instr   1
                ; print p1, p2, p3, p4, p5
                ; Slider sends note with MIDI key number.
                ; Convert it to octaves.
                ioctave = p4 / 12.0 + 3.0
                print ioctave
                ; Convert octave to frequency.
                ihertz = cpsoct(ioctave)
                p3 = p3 + 0.05
            a1  oscili  ampdb(p5), ihertz, 1
                kenv linseg 0, 0.05, 1, p3, 0
                a1 = kenv * a1
                outs    a1,a1
                endin
            ''')
            # And a score.
            self.csound.setScore('''
            f1 0 8192 10 1
            f0 600
            e
            ''')
            # Real-time audio output.
            # It is not necessary to enable line events.
            self.csound.setCommand('''csound -h -m128 -d -odac -B512 -b400 temp.orc temp.sco''')
            # Export the orc and sco.
            self.csound.exportForPerformance()
            # Start the performance.
            self.csound.compile()
            # Perform in blocks of ksmps
            # (Csound will yield implicitly to wxWindows).
            self.performanceThread = csnd6.CsoundPerformanceThread(self.csound)
            self.performanceThread.Play()

        # Handle the button click -- send a note to Csound
        # with the pitch set by the slider.
        def OnClickButton1(self, event):
            print "You pressed button 1"
            # Send a line event.
            self.performanceThread.InputMessage("i 1 0 8 %i 70" % self.pitch)

        # Handle the slider movement -- change the pitch.
        def OnSlider1Move(self, event):
            print "You moved the slider to %d" % event.GetInt()
            self.pitch = event.GetInt()

        # Handle the window close event -- stop performance and
        # wait for Csound to exit, then destroy main window.
        def OnClose(self, event):
            try:
                print "You closed the window."
                self.performanceThread.Stop()
                self.performanceThread.Join()
                self.csoundThread.join()
                print 'Csound thread has finished.'
                self.GetParent().Destroy()
            except:
                print traceback.print_exc()

try:
    # Create a wx application.
    application = wx.PySimpleApp()
    # Create a frame to hold the control panel.
    frame = wx.Frame(None, -1, "Csound Controller")
    # Create the control panel as a child of the frame.
    controlPanel = ControlPanel(frame)
    # Display the frame.
    frame.Show(True)
    # Run the application.
    application.MainLoop()
    print 'Done...'
except:
    traceback.print_exc()
