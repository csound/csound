/*
 
 CsoundVirtualKeyboard.swift
 
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

class CsoundVirtualKeyboard: UIView {
    
    // Variable declarations
    private var keyDown = [Bool].init(repeating: false, count: 25)
    private var currentTouches = NSMutableSet()
    private var keyRects = [CGRect](repeatElement(CGRect(), count: 25))
    private var lastWidth: CGFloat = 0.0
    private let keysNum = 25
    
    @IBOutlet var keyboardDelegate: AnyObject? // keyboardDelegate object will handle key presses and releases
    
    override init(frame: CGRect) {
        super.init(frame: frame)
    }
    
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        isMultipleTouchEnabled = true   // Enable multiple touch so we can press multiple keys
    }
    
    // Check if a key is a white key or a black key depending on its index
    func isWhiteKey(_ key: Int) -> Bool {
        switch (key % 12) {
        case 0, 2, 4, 5, 7, 9, 11:
            return true
        default:
            return false
        }
    }
    
    // MARK: Update key statuses
    private func updateKeyRects() {
        if lastWidth == bounds.size.width { return }
        
        lastWidth = bounds.size.width
        let whiteKeyHeight: CGFloat = bounds.size.height
        let blackKeyHeight: CGFloat = whiteKeyHeight * 0.625
        
        let whiteKeyWidth: CGFloat = bounds.size.width/15.0
        let blackKeyWidth: CGFloat = whiteKeyWidth * 0.833333
        
        let leftKeyBound: CGFloat = whiteKeyWidth - (blackKeyWidth / 2.0)
        
        var lastWhiteKey: CGFloat = 0
        
        keyRects[0] = CGRect(x: 0, y: 0, width: whiteKeyWidth, height: whiteKeyHeight)
        
        for i in 1 ..< keysNum {
            if !(isWhiteKey(i)) {
                keyRects[i] = CGRect(x: (lastWhiteKey * whiteKeyWidth) + leftKeyBound, y: 0, width: blackKeyWidth, height: blackKeyHeight)
            } else {
                lastWhiteKey += 1
                keyRects[i] = CGRect(x: lastWhiteKey * whiteKeyWidth, y: 0, width: whiteKeyWidth, height: whiteKeyHeight)
            }
        }
    }
    
    private func getKeyboardKey(_ point: CGPoint) -> Int {
        var keyNum = -1
        
        for i in 0 ..< keysNum {
            if keyRects[i].contains(point) {
                keyNum = i
                if !(isWhiteKey(i)) {
                    break
                }
            }
        }
        return keyNum
    }
    
    private func updateKeyStates() {
        updateKeyRects()
        
        var count = 0
        
        let touches = currentTouches.allObjects
        count = touches.count
        
        var currentKeyState = [Bool](repeatElement(true, count: keysNum))
        
        for i in 0 ..< keysNum {
            currentKeyState[i] = false
        }
        
        for i in 0 ..< count {
            let touch = touches[i] as! UITouch
            let point = touch.location(in: self)
            let index = getKeyboardKey(point)
            
            if index != -1 {
                currentKeyState[index] = true
            }
        }
        
        var keysUpdated = false
        
        for i in 0 ..< keysNum {
            if keyDown[i] != currentKeyState[i] {
                keysUpdated = true
            
                let keyDownState = currentKeyState[i]
                keyDown[i] = keyDownState
                
                if keyboardDelegate != nil {
                    if keyDownState {
                        keyboardDelegate?.keyDown(self, keyNum: i)
                    } else {
                        keyboardDelegate?.keyUp(self, keyNum: i)
                    }
                }
            }
        }
        
        if keysUpdated {
            DispatchQueue.main.async { [unowned self] in
                self.setNeedsDisplay()
            }
        }
    }

    // MARK: Touch Handling Code
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            currentTouches.add(touch)
        }
        updateKeyStates()
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        updateKeyStates()
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            currentTouches.remove(touch)
        }
        updateKeyStates()
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            currentTouches.remove(touch)
        }
        updateKeyStates()
    }
    
    // MARK: Drawing Code
    override func draw(_ rect: CGRect) {
        let context = UIGraphicsGetCurrentContext()
        
        let whiteKeyHeight = rect.size.height
        let blackKeyHeight = round(rect.size.height * 0.625)
        let whiteKeyWidth = rect.size.width/15.0
        let blackKeyWidth = whiteKeyWidth * 0.8333333
        let blackKeyOffset = blackKeyWidth/2.0
        
        var runningX: CGFloat = 0
        let yval = 0
        
        context?.setFillColor(UIColor.white.cgColor)
        context?.fill(bounds)
        
        context?.setStrokeColor(UIColor.black.cgColor)
        context?.stroke(bounds)
        
        let lineHeight = whiteKeyHeight - 1
        var newX = 0
        
        for i in 0 ..< keysNum {
            if isWhiteKey(i) {
                newX = Int(round(runningX + 0.5))
                
                if keyDown[i] {
                    let newW = Int(round(runningX + whiteKeyWidth + 0.5) - CGFloat(newX))
                    context?.setFillColor(UIColor.blue.cgColor)
                    context?.fill(CGRect(x: CGFloat(newX), y: CGFloat(yval), width: CGFloat(newW), height: whiteKeyHeight - 1))
                }
                runningX += whiteKeyWidth
                
                context?.setStrokeColor(UIColor.black.cgColor)
                context?.stroke(CGRect(x: CGFloat(newX), y: CGFloat(yval), width: CGFloat(newX), height: lineHeight))
            }
        }
        
        runningX = 0
        
        for i in 0 ..< keysNum {
            if isWhiteKey(i) {
                runningX += whiteKeyWidth
            } else {
                if keyDown[i] {
                    context?.setFillColor(UIColor.blue.cgColor)
                } else {
                    context?.setFillColor(UIColor.black.cgColor)
                }
                
                context?.fill(CGRect(x: runningX - blackKeyOffset, y: CGFloat(yval), width: blackKeyWidth, height: blackKeyHeight))
                context?.setStrokeColor(UIColor.black.cgColor)
                context?.stroke(CGRect(x: runningX - blackKeyOffset, y: CGFloat(yval), width: blackKeyWidth, height: blackKeyHeight))
            }
        }
    }
}

// Protocol for delegate to conform to
@objc protocol CsoundVirtualKeyboardDelegate {
    func keyUp(_ keybd:CsoundVirtualKeyboard, keyNum:Int)
    func keyDown(_ keybd:CsoundVirtualKeyboard, keyNum:Int)
}
