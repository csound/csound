/*
 
 CsoundHaiku4ViewController.swift
 
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
import SafariServices   // For SFSafariViewController

class CsoundHaiku4ViewController: BaseCsoundViewController {

    override func viewDidLoad() {
        title = "05. Play: Haiku IV"
        super.viewDidLoad()
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        let tempFile = Bundle.main.path(forResource: "IV", ofType: "csd")
        
        csound.stop()
        csound = CsoundObj()
        csound.play(tempFile)
    }
    
    // Use SFSafariViewController to display Iain McCurdy's web-page about the piece
    @IBAction func showSite(_ sender: UIButton) {
        let url: URL! = URL(string: "http://iainmccurdy.org/csoundhaiku.html")
        let safariVC = SFSafariViewController(url: url)
        present(safariVC, animated: true, completion: nil)
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 180)
        infoText = "Haiku IV is the fourth in a suite of nine generative Csound pieces by Iain McCurdy. Csound begins rendering the work when the view appears and stops when the view unloads and CsoundObj is deallocated."
        displayInfo(sender)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}
