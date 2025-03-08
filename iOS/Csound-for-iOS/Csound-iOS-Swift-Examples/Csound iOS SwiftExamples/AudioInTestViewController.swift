/*
 
 AudioInTestViewController.swift
 
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

class AudioInTestViewController: BaseCsoundViewController {
    
    @IBOutlet var mLeftDelayTimeSlider: UISlider!
    @IBOutlet var mLeftFeedbackSlider: UISlider!
    @IBOutlet var mRightDelayTimeSlider: UISlider!
    @IBOutlet var mRightFeedbackSlider: UISlider!
    @IBOutlet var mSwitch: UISwitch!

    override func viewDidLoad() {
        title = "11. Mic: Stereo Delay"
        super.viewDidLoad()
    }
    
    @IBAction func toggleOnOff(_ sender: UISwitch) {
        print("Status: \(sender.isOn)")
        
        if sender.isOn {
            let tempFile = Bundle.main.path(forResource: "audioInTest", ofType: "csd")
            
            csound.stop()
            csound = CsoundObj()
            csound.useAudioInput = true
            csound.add(self)
            
            let csoundUI: CsoundUI = CsoundUI(csoundObj: csound)
            csoundUI.add(mLeftDelayTimeSlider, forChannelName: "leftDelayTime")
            csoundUI.add(mLeftFeedbackSlider, forChannelName: "leftFeedback")
            csoundUI.add(mRightDelayTimeSlider, forChannelName: "rightDelayTime")
            csoundUI.add(mRightFeedbackSlider, forChannelName: "rightFeedback")
            
            csound.play(tempFile)
        } else {
            csound.stop()
        }
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 120)
        infoText = "This example shows audio processing in real-time with independent delay-time and feedback settings for each channel."
        displayInfo(sender)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

extension AudioInTestViewController: CsoundObjListener {
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in
            self.mSwitch.isOn = false
        }
    }
}
