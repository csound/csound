/*
 
 SimpleTest2ViewController.swift
 
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

class SimpleTest2ViewController: BaseCsoundViewController {
    
    @IBOutlet var onOffSwitch: UISwitch!
    
    @IBOutlet var rateSlider: UISlider!
    @IBOutlet var durationSlider: UISlider!
    @IBOutlet var rateLabel: UILabel!
    @IBOutlet var durationLabel: UILabel!
    
    @IBOutlet var attackSlider: UISlider!
    @IBOutlet var decaySlider: UISlider!
    @IBOutlet var sustainSlider: UISlider!
    @IBOutlet var releaseSlider: UISlider!
    
    @IBOutlet var attackLabel: UILabel!
    @IBOutlet var decayLabel: UILabel!
    @IBOutlet var sustainLabel: UILabel!
    @IBOutlet var releaseLabel: UILabel!

    override func viewDidLoad() {
        title = "02. Simple Test 2"
        super.viewDidLoad()
    }
    
    func toggleOnOff(_ sender: UISwitch) {
        if sender.isOn {
            let tempFile = Bundle.main.path(forResource: "test2", ofType: "csd")
            
            csound.stop()
            csound = CsoundObj()
            csound.add(self)
            
            // Add UI bindings using CsoundUI, a class that binds Csound values to UIKit objects
            let csoundUI = CsoundUI(csoundObj: csound)
            csoundUI?.labelPrecision = 2
            csoundUI?.add(rateSlider, forChannelName: "noteRate")
            csoundUI?.add(durationSlider, forChannelName: "duration")
            csoundUI?.add(attackSlider, forChannelName: "attack")
            csoundUI?.add(decaySlider, forChannelName: "decay")
            csoundUI?.add(sustainSlider, forChannelName: "sustain")
            csoundUI?.add(releaseSlider, forChannelName: "release")
            
            csoundUI?.add(rateLabel, forChannelName: "noteRate")
            csoundUI?.add(durationLabel, forChannelName: "duration")
            csoundUI?.add(attackLabel, forChannelName: "attack")
            csoundUI?.add(decayLabel, forChannelName: "decay")
            csoundUI?.add(sustainLabel, forChannelName: "sustain")
            csoundUI?.add(releaseLabel, forChannelName:  "release")
            
            csound.play(tempFile)
        } else {
            csound.stop()
        }
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 140)
        infoText = "A generative music example that contains a number of sliders that affect the rate, duration, and envelope of each note."
        displayInfo(sender)
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

extension SimpleTest2ViewController: CsoundObjListener {
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in
            self.onOffSwitch.isOn = false
        }
    }
}
