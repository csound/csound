/*
 
 HardwareTestViewController.swift
 
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
import CoreMotion

class HardwareTestViewController: BaseCsoundViewController {
    
    @IBOutlet var mSwitch: UISwitch!
    
    // Labels to display accelerometer, gyro, attitude sensor values
    @IBOutlet var accX: UILabel!
    @IBOutlet var accY: UILabel!
    @IBOutlet var accZ: UILabel!
    
    @IBOutlet var gyroX: UILabel!
    @IBOutlet var gyroY: UILabel!
    @IBOutlet var gyroZ: UILabel!
    
    @IBOutlet var roll: UILabel!
    @IBOutlet var pitch: UILabel!
    @IBOutlet var yaw: UILabel!
    
    var csoundMotion = CsoundMotion()
    var motionManager = CMMotionManager()
    
    override func viewDidLoad() {
        title = "14, Hardware: Motion Control"
        super.viewDidLoad()
    }
    
    @IBAction func toggleOnOff(_ sender: UISwitch) {
        if sender.isOn {
            let tempFile = Bundle.main.path(forResource: "hardwareTest", ofType: "csd")
            
            csound.stop()
            csound = CsoundObj()
            csound.add(self)
            
            csoundMotion = CsoundMotion(csoundObj: csound)
            
            if csoundMotion.motionManager != nil {
                motionManager = csoundMotion.motionManager  // Grab the csoundMotion object's CMMotionManager instance
                
                // Enable accelerometer
                if motionManager.isAccelerometerAvailable {
                    csoundMotion.enableAccelerometer()
                    motionManager.accelerometerUpdateInterval = 0.1 // Set how quickly updates are pulled
                    motionManager.startAccelerometerUpdates(to: .main, withHandler: { [unowned self] (data: CMAccelerometerData?, error: Error?) in
                        if data?.acceleration != nil {  // Use a closure to publish sensor values to labels
                            self.accX.text = String(format: "%.3f", (data?.acceleration.x)!)
                            self.accY.text = String(format: "%.3f", (data?.acceleration.y)!)
                            self.accZ.text = String(format: "%.3f", (data?.acceleration.z)!)
                        }
                    })
                }
                
                // Enable gyro
                if motionManager.isGyroAvailable {
                    csoundMotion.enableGyroscope()
                    motionManager.gyroUpdateInterval = 0.1
                    motionManager.startGyroUpdates(to: .main, withHandler: { [unowned self] (data: CMGyroData?, error: Error?) in
                        if data?.rotationRate != nil {
                            self.gyroX.text = String(format: "%.3f", (data?.rotationRate.x)!)
                            self.gyroY.text = String(format: "%.3f", (data?.rotationRate.y)!)
                            self.gyroZ.text = String(format: "%.3f", (data?.rotationRate.z)!)
                        }
                    })
                }
                
                // Enable attitude
                if motionManager.isDeviceMotionAvailable {
                    csoundMotion.enableAttitude()
                    motionManager.accelerometerUpdateInterval = 0.1
                    motionManager.startDeviceMotionUpdates(to: .main, withHandler: { [unowned self] (data: CMDeviceMotion?, error: Error?) in
                        if data?.attitude != nil {
                            self.roll.text = String(format: "%.3f", (data?.attitude.roll)!)
                            self.pitch.text = String(format: "%.3f", (data?.attitude.pitch)!)
                            self.yaw.text = String(format: "%.3f", (data?.attitude.yaw)!)
                        }
                    })
                }
            }
            csound.play(tempFile)
        } else {
            csound.stop()
            
            // Stop sensor updates
            if motionManager == csoundMotion.motionManager {
                motionManager.stopAccelerometerUpdates()
                motionManager.stopGyroUpdates()
                motionManager.stopDeviceMotionUpdates()
            }
        }
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 220)
        infoText = "Hardware: Motion Control shows how to use the device's motion sensor data as a set of controllers for Csound, and also displays this data in a set of UILabels. Accelerometer X controls oscillator frequency, Attitude: Yaw controls filter cutoff, Attitude: Pitch controls amplitude, and Attitude: Roll controls filter resonance."
        displayInfo(sender)
    }
}

extension HardwareTestViewController: CsoundObjListener {
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in
            self.mSwitch.isOn = false
        }
    }
}
