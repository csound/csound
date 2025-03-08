/*
 
 InstrumentEditorViewController.swift
 
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

class InstrumentEditorViewController: BaseCsoundViewController {
    
    @IBOutlet var orchestraTextView: UITextView!

    override func viewDidLoad() {
        title = "08, Instrument Tweaker"
        super.viewDidLoad()
        
        let csdFile = Bundle.main.path(forResource: "instrumentEditor", ofType: "csd")
        csound.stop()
        csound.play(csdFile)
    }
    
    // Trigger an instance of the instrument
    @IBAction func trigger(_ sender: UIButton) {
        csound.updateOrchestra(orchestraTextView.text)
        csound.sendScore("i1 0 1")
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 120)
        infoText = "This example allows the user to modify the contents of the .csd on-the-fly using the updateOrchestra method from CsoundObj."
        displayInfo(sender)
    }

}
