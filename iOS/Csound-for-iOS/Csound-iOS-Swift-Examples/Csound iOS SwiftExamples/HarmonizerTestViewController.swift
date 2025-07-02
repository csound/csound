/*
 
 HarmonizerTestViewController.swift
 
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

class HarmonizerTestViewController: BaseCsoundViewController {
    
    @IBOutlet var mHarmPitchSlider: UISlider!
    @IBOutlet var mGainSlider: UISlider!
    @IBOutlet var mSwitch: UISwitch!

    override func viewDidLoad() {
        title = "12. Mic: Harmonizer"
        super.viewDidLoad()
    }
    
    @IBAction func toggleOnOff(_ sender: UISwitch) {
        if sender.isOn {
            let tempFile = Bundle.main.path(forResource: "harmonizer", ofType: "csd")
            
            csound.stop()
            csound = CsoundObj()
            csound.useAudioInput = true
            csound.add(self)
            
            let csoundUI = CsoundUI(csoundObj: csound)
            csoundUI?.add(mHarmPitchSlider, forChannelName: "slider")
            csoundUI?.add(mGainSlider, forChannelName: "gain")
            
            csound.play(tempFile)
        } else {
            csound.stop()
        }
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 120)
        infoText = "This examples uses Csound's streaming phase vocoder to create a harmonizer effect. A dry/wet balance control is provided."
        displayInfo(sender)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

extension HarmonizerTestViewController: CsoundObjListener {
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in
            self.mSwitch.isOn = false
        }
    }
}
