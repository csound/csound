/*
 
 SimpleTest1ViewController.swift

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

class SimpleTest1ViewController: BaseCsoundViewController {
    
    @IBOutlet var uiSlider: UISlider!
    @IBOutlet var uiSwitch: UISwitch!
    @IBOutlet var uiLabel: UILabel!

    override func viewDidLoad() {
        title = "01. Simple Test 1"
        super.viewDidLoad()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }
    
    @IBAction func toggleOnOff(_ sender: UISwitch) {
        print("Status: \(sender.isOn)")
        
        if sender.isOn {
            let csdFile = Bundle.main.path(forResource: "test", ofType: "csd")
            
            csound = CsoundObj()
            csound.add(self)    // Add self as a 'listener', a kind of delegate, to be notified when Csound starts and/or stops
            let csoundUI = CsoundUI(csoundObj: csound)
            csoundUI?.labelPrecision = 2    // Set label value display precision before adding UILabel binding
            csoundUI?.add(uiLabel, forChannelName: "slider")
            csoundUI?.add(uiSlider, forChannelName: "slider")
            
            csound.play(csdFile)
        } else {
            csound.stop()
        }
    }
    
    // Present info popover
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 200, height: 200)
        infoText = "Flip the switch to begin rendering Csound. Use the slider to control pitch."
        displayInfo(sender) // Call inherited method to display info popover after setting specifics
    }
}

// CsoundObjListener method: if Csound finishes running, it will call this method
extension SimpleTest1ViewController: CsoundObjListener {
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in    // Use the main thread for UI operation
            self.uiSwitch.isOn = false  // Turn the switch off
            self.uiLabel.text = ""
        }
    }
}
