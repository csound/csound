/*
 
 WaveviewViewController.swift

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

class WaveviewViewController: BaseCsoundViewController {
    
    @IBOutlet var titleLabel: UILabel!
    @IBOutlet var waveview: Waveview!
    
    var fTableIndex = 0
    let fTables = ["Sine", "Exponential Curves", "Data Points", "Normalizing Function", "Triangle"]

    override func viewDidLoad() {
        title = "09. F-table Viewer"
        super.viewDidLoad()
    }
    
    override func viewDidAppear(_ animated: Bool) {
        let tempFile = Bundle.main.path(forResource: "Waveviewtest", ofType: "csd")
        
        csound.stop()
        csound = CsoundObj()
        csound.addBinding(waveview)
        
        // Add a tap gesture recognizer to increment F-table by tapping the Waveview
        let tap = UITapGestureRecognizer(target: self, action: #selector(incrementFTable(_:)))
        waveview.addGestureRecognizer(tap)
        
        csound.play(tempFile)
    }
    
    // Cycle through available F-tables
    @IBAction func incrementFTable(_ tap: UITapGestureRecognizer) {
        fTableIndex += 1
        fTableIndex %= fTables.count
        titleLabel.text = fTables[fTableIndex]
        waveview.displayFTable(fTableIndex+1)   // Offset by 1 to match F-table numbering in Csound
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 120)
        infoText = "Demonstrates using a Csound binding to draw F-table content to a view. Tap the screen to cycle through available F-tables."
        displayInfo(sender)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}
