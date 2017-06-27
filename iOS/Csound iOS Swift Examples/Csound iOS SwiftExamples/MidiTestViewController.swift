/*
 
 MidiTestViewController.swift
 
 Nikhil Singh, Dr. Richard Boulanger
 Adapted from the Csound iOS Examples by Steven Yi and Victor Lazzarini
 
 This file is part of Csound iOS SwiftExamples.
 
 The Csound for iOS Library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

import UIKit

class MidiTestViewController: BaseCsoundViewController {
    
    @IBOutlet var mAttackSlider: UISlider!
    @IBOutlet var mDecaySlider: UISlider!
    @IBOutlet var mSustainSlider: UISlider!
    @IBOutlet var mReleaseSlider: UISlider!
    
    @IBOutlet var mCutoffSlider: UISlider!
    @IBOutlet var mResonanceSlider: UISlider!
    
    @IBOutlet var mSwitch: UISwitch!
    let widgetsManager = MidiWidgetsManager()

    override func viewDidLoad() {
        title = "04. Hardware: MIDI Controller"
        
        // Use a MidiWidgetsManager object to add MIDI CC to UI object bindings
        widgetsManager.add(mAttackSlider, forControllerNumber: 1)
        widgetsManager.add(mDecaySlider, forControllerNumber: 2)
        widgetsManager.add(mSustainSlider, forControllerNumber: 3)
        widgetsManager.add(mReleaseSlider, forControllerNumber: 4)
        widgetsManager.add(mCutoffSlider, forControllerNumber: 5)
        widgetsManager.add(mResonanceSlider, forControllerNumber: 6)
        widgetsManager.openMidiIn()
        
        super.viewDidLoad()
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        widgetsManager.closeMidiIn()    // Close MIDI input
        super.viewWillDisappear(animated)
    }
    
    @IBAction func toggleOnOff(_ sender: UISwitch) {
        if sender.isOn {
            let tempFile = Bundle.main.path(forResource: "midiTest", ofType: "csd")
            
            csound.stop()
            csound = CsoundObj()
            csound.add(self)
            
            // Add value bindings for UI objects which are controlled by MIDI CCs
            let csoundUI = CsoundUI(csoundObj: csound)
            csoundUI?.add(mAttackSlider, forChannelName: "attack")
            csoundUI?.add(mDecaySlider, forChannelName: "decay")
            csoundUI?.add(mSustainSlider, forChannelName: "sustain")
            csoundUI?.add(mReleaseSlider, forChannelName: "release")
            csoundUI?.add(mCutoffSlider, forChannelName: "cutoff")
            csoundUI?.add(mResonanceSlider, forChannelName: "resonance")
            
            csound.midiInEnabled = true
            csound.play(tempFile)
        } else {
            csound.stop()
        }
    }
    
    // Turn off all notes using a csound instrument
    @IBAction func midiPanic(_ sender: UIButton) {
        csound.sendScore("i \"allNotesOff\" 0 1")
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 110)
        infoText = "This example demonstrate MIDI input from hardware, as well an on-screen (simulated) MIDI keyboard."
        displayInfo(sender)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

extension MidiTestViewController: CsoundObjListener {
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in
            self.mSwitch.isOn = false
        }
    }
}

// Virtual keyboard delegate methods
extension MidiTestViewController: CsoundVirtualKeyboardDelegate {
    func keyDown(_ keybd: CsoundVirtualKeyboard, keyNum: Int) {
        let midikey = 60 + keyNum
        csound.sendScore(String(format: "i1.%003d 0 -1 \(midikey) 0", midikey))
    }
    
    func keyUp(_ keybd: CsoundVirtualKeyboard, keyNum: Int) {
        let midikey = 60 + keyNum
        csound.sendScore(String(format: "i-1.%003d 0 0", midikey))
    }
}
