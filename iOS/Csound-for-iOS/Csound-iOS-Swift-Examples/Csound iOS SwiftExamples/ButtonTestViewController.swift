/*
 
 ButtonTestViewController.swift
 
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

class ButtonTestViewController: BaseCsoundViewController {
    
    @IBOutlet var mValueButton: UIButton!
    @IBOutlet var mEventButton: UIButton!
    
    @IBOutlet var mDurationSlider: UISlider!
    
    @IBOutlet var mAttackSlider: UISlider!
    @IBOutlet var mDecaySlider: UISlider!
    @IBOutlet var mSustainSlider: UISlider!
    @IBOutlet var mReleaseSlider: UISlider!
    @IBOutlet var mSwitch: UISwitch!
    
    @IBAction func toggleOnOf(_ sender: UISwitch) {
        if sender.isOn {
            let tempFile = Bundle.main.path(forResource: "buttonTest", ofType: "csd")
            csound = CsoundObj()
            csound.add(self)
            
            let csoundUI = CsoundUI(csoundObj: csound)
            csoundUI?.addMomentaryButton(mValueButton, forChannelName: "button1")
            csoundUI?.add(mDurationSlider, forChannelName: "duration")
            csoundUI?.add(mAttackSlider, forChannelName: "attack")
            csoundUI?.add(mDecaySlider, forChannelName: "decay")
            csoundUI?.add(mSustainSlider, forChannelName: "sustain")
            csoundUI?.add(mReleaseSlider, forChannelName: "release")
            
            csound.play(tempFile)
        } else {
            csound.stop()
        }
    }
    
    @IBAction func eventButtonHit(_ sender: UIButton) {
        let score = "i2 0 \(mDurationSlider.value)"
        csound.sendScore(score)
    }

    override func viewDidLoad() {
        title = "03. Button Test"
        super.viewDidLoad()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 160)
        infoText = "Uses a .csd based on SimpleTest 2, but depends on the user to press a button to trigger each note. One button uses a binding and the other sends a score message to CsoundObj."
        displayInfo(sender)
    }
}

extension ButtonTestViewController: CsoundObjListener {
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in
            self.mSwitch.isOn = false
        }
    }
}
