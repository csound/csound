/*
 
 MasterViewController.swift
 
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

class MasterViewController: UITableViewController {

    var detailViewController: BaseCsoundViewController? = nil
    
    // All ViewController Classes
    var allVCClasses: [BaseCsoundViewController.Type] = [SimpleTest1ViewController.self,
                                                         SimpleTest2ViewController.self,
                                                         ButtonTestViewController.self,
                                                         CsoundHaiku4ViewController.self,
                                                         TrappedGeneratorViewController.self,
                                                         ConsoleOutputViewController.self,
                                                         InstrumentEditorViewController.self,
                                                         WaveviewViewController.self,
                                                         AudioFilesTestViewController.self,
                                                         AudioInTestViewController.self,
                                                         HarmonizerTestViewController.self,
                                                         RecordTestViewController.self,
                                                         MidiTestViewController.self,
                                                         HardwareTestViewController.self,
                                                         MultiTouchXYViewController.self,
                                                         PitchShifterViewController.self]
    
    // Display titles for ViewControllers, and for master TableView cells
    let testNames = ["01. Simple Test 1",
                    "02. Simple Test 2",
                    "03. Button Test",
                    "04. Play: Haiku IV",
                    "05. Render: Trapped in Convert",
                    "06. Render: Console Output",
                    "07. Instrument Tweaker",
                    "08. F-table Viewer",
                    "09. Soundfile: Pitch Shifter",
                    "10. Mic: Stereo Delay",
                    "11. Mic: Harmonizer",
                    "12. Mic: Recording",
                    "13. Hardware: MIDI Controller",
                    "14. Hardware: Motion Control",
                    "15. XY Pad: MultiTouch ",
                    "16. XY Pad: Mic PitchShift+Mix"]

    override func viewDidLoad() {
        super.viewDidLoad()
        title = "Csound for iOS"
        if let split = splitViewController {
            let controllers = split.viewControllers
            detailViewController = (controllers[controllers.count-1] as! UINavigationController).topViewController as? BaseCsoundViewController
            tableView.selectRow(at: IndexPath.init(row: 0, section: 0), animated: false, scrollPosition: .middle)
        }
    }

    override func viewWillAppear(_ animated: Bool) {
        clearsSelectionOnViewWillAppear = splitViewController!.isCollapsed
        super.viewWillAppear(animated)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    // MARK: - Table View
    override func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return testNames.count
    }
    
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath)

        cell.textLabel!.text = testNames[indexPath.row]
        return cell
    }
    
    // Ensure contents are not editable
    override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        return false
    }
    
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        var controller: Any?
        
        if indexPath.row == 12, UIDevice.current.userInterfaceIdiom == .pad {   // Separate .xib for iPad for MIDI Test example (with virtual keyboard)
            controller = allVCClasses[indexPath.row].init(nibName: String(describing: allVCClasses[indexPath.row]) + "_iPad", bundle: nil)
        } else if indexPath.row == 0 {  // SimpleTest1 UI is housed in Main.storyboard
            controller = UIStoryboard.init(name: "Main", bundle: nil).instantiateViewController(withIdentifier: "SimpleTest1ViewController")
        } else {    // Else, use the classes from the allVCClasses array and their names
            controller = allVCClasses[indexPath.row].init(nibName: String(describing: allVCClasses[indexPath.row]), bundle: nil)
        }
        
        if controller != nil {
            if UIDevice.current.userInterfaceIdiom == .phone {  // Push VC if on iPhone
                navigationController?.pushViewController(controller as! BaseCsoundViewController, animated: true)
            } else {    // SplitViewController management on iPad
                let navCon = UINavigationController(rootViewController: controller as! BaseCsoundViewController)
                splitViewController?.viewControllers = [navigationController!, navCon]
                splitViewController?.delegate = controller as! BaseCsoundViewController
                
                if UIDevice.current.orientation == .portrait {  // If iPad orientation is portrait, animate hiding the master pane
                    UIView.animate(withDuration: 0.2, animations: { [unowned self] in
                        self.splitViewController?.preferredDisplayMode = .primaryHidden
                    })
                }
            }
        }
    }


}

