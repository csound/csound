/*
 
 AudioFilesTestViewController.swift
 
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

class AudioFilesTestViewController: BaseCsoundViewController {
    
    @IBOutlet var playButton: UIButton!
    @IBOutlet var pitchKnob: ControlKnob!
    @IBOutlet var pitchLabel: UILabel!
    
    @IBAction func play(_ sender: UIButton) {
        let audioFilePath = Bundle.main.path(forResource: "testAudioFile", ofType: "aif")
        if audioFilePath != nil {
            let score = "i1 0 1 \"\(audioFilePath!)\""
            csound.sendScore(score)
        }
    }
    
    @IBAction func changePitch(_ sender: ControlKnob) {
        pitchLabel.text = String(format: "%.2f", sender.value)
    }
    
    @IBAction func stop(_ sender: UIButton) {
        csound.sendScore("i3 0 1 2")
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 140)
        infoText = "Soundfile PitchShifter uses the URL of a bundled AIFF file and playing it with Csound. Also demonstrated is a custom UI control knob widget, used to change playback pitch."
        displayInfo(sender)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "10. Soundfile: Pitch Shifter"
        csound = CsoundObj()
        let csdPath = Bundle.main.path(forResource: "audiofiletest", ofType: "csd")
        
        pitchKnob.minimumValue = 0.5
        pitchKnob.maximumValue = 2.0
        pitchKnob.value = 1.0
        
        csound.addBinding(self.pitchKnob)
        csound.play(csdPath)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}
