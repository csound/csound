/*
 
 RecordTestViewController.swift
 
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
import AVFoundation // For AVAudioPlayer

class RecordTestViewController: BaseCsoundViewController {
    
    @IBOutlet var mSwitch: UISwitch!
    @IBOutlet var mGainSlider: UISlider!
    @IBOutlet var mGainLabel: UILabel!
    @IBOutlet var mLevelMeter: LevelMeterView!
    @IBOutlet var mPlayButton: UIButton!
    
    var hasRecorded = false
    var mPlayer = AVAudioPlayer()   // To play recorded file

    override func viewDidLoad() {
        title = "13. Mic: Recording"
        super.viewDidLoad()
    }
    
    @IBAction func toggleOnOff(_ sender: UISwitch) {
        if sender.isOn {
            let tempFile = Bundle.main.path(forResource: "recordTest", ofType: "csd")
            
            csound.stop()
            csound = CsoundObj()
            csound.useAudioInput = true // We must set this property of CsoundObj to true to use audio input
            csound.add(self)
            
            let csoundUI = CsoundUI(csoundObj: csound)
            csoundUI?.add(mGainSlider, forChannelName: "gain")
            
            mLevelMeter.add(to: csound, for: "meter")
            csound.play(tempFile)
        } else {
            csound.stopRecording()
            csound.stop()
        }
    }
    
    @IBAction func changeGain(_ sender: UISlider) {
        mGainLabel.text = String(format: "%.2f", sender.value)
    }
    
    @IBAction func play(_ sender: UIButton) {
        if hasRecorded {
            mPlayer.prepareToPlay()
            mPlayer.play()
            sender.removeTarget(self, action: #selector(play(_:)), for: .touchUpInside)
            sender.addTarget(self, action: #selector(stop(_:)), for: .touchUpInside)
            sender.setTitle("Stop", for: .normal)
        }
    }
    
    @IBAction func stop(_ sender: UIButton) {
        mPlayer.stop()
        mPlayer.currentTime = 0
        sender.removeTarget(self, action: #selector(stop(_:)), for: .touchUpInside)
        sender.addTarget(self, action: #selector(play(_:)), for: .touchUpInside)
        sender.setTitle("Play", for: .normal)
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    func recordingURL() -> URL {
        let docDirPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        return docDirPath.appendingPathComponent("recording.wav")
    }
    
    @IBAction func showInfo(_ sender: UIButton) {
        infoVC.preferredContentSize = CGSize(width: 300, height: 120)
        infoText = "This example uses a custom level-meter widget, and demonstrates efficient use of concurrency for an audio application."
        displayInfo(sender)
    }
}

extension RecordTestViewController: CsoundObjListener {
    func csoundObjStarted(_ csoundObj: CsoundObj!) {
        csound.record(to: recordingURL())
    }
    
    func csoundObjCompleted(_ csoundObj: CsoundObj!) {
        DispatchQueue.main.async { [unowned self] in
            self.mSwitch.isOn = false
        }
        
        do {
            mPlayer = try AVAudioPlayer(contentsOf: recordingURL())
        } catch {
            print(error.localizedDescription)
        }
        
        mPlayer.delegate = self
        hasRecorded = true
    }
}

extension RecordTestViewController: AVAudioPlayerDelegate {
    func audioPlayerDidFinishPlaying(_ player: AVAudioPlayer, successfully flag: Bool) {
        stop(mPlayButton)
    }
}
