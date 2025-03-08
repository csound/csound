/*
 
 Waveview.swift

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

class Waveview: UIView {

    private var lastY: CGFloat = 0
    private var displayData = [Float]()
    fileprivate var table: UnsafeMutablePointer<Float>?
    fileprivate var tableLength = 0
    fileprivate var fTableNumber = 0
    fileprivate var csObj = CsoundObj()
    fileprivate var tableLoaded = false
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        displayData = [Float](repeatElement(Float(0), count: Int(frame.width))) // Init with 0s
    }
    
    required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        displayData = [Float](repeatElement(Float32(0), count: Int(frame.width)))
    }
    
    func displayFTable(_ fTableNum: Int) {
        fTableNumber = fTableNum
        tableLoaded = false
        self.updateValuesFromCsound()
    }
    
    // Take in a pointer to a float and return a Swift array of values stored beginning at that memory location
    func ptoa(_ ptr: UnsafeMutablePointer<Float>, length: Int) -> [Float] {
        let bfr = UnsafeBufferPointer(start: ptr, count: length)
        return [Float](bfr)
    }
    
    // MARK: Drawing Code
    override func draw(_ rect: CGRect) {
        let context = UIGraphicsGetCurrentContext()
        context?.setFillColor(UIColor.black.cgColor)
        context?.fill(rect)
        
        if tableLoaded {
            context?.setStrokeColor(UIColor.white.cgColor)
            context?.setFillColor(UIColor.white.cgColor)
            let fill_path = CGMutablePath()
            let x: CGFloat = 0
            let y: CGFloat = CGFloat(displayData[0])
            
            fill_path.move(to: CGPoint(x: x, y: y), transform: .identity)
            
            for i in 0 ..< displayData.count {
                fill_path.addLine(to: CGPoint(x: CGFloat(i), y: CGFloat(displayData[i])), transform: .identity)
            }
            
            context?.addPath(fill_path)
            context?.setAllowsAntialiasing(true)
            context?.drawPath(using: .stroke)
        }
    }
    
    // Update values from F-table in displayData array
    func updateDisplayData() {
        let scalingFactor: Float32 = 0.9
        let width = self.frame.size.width
        let height = self.frame.size.height
        let middle: Float32 = Float32(height / 2.0)
        
        if table != nil {
            let valTable = ptoa(table!, length: tableLength)
            displayData = [Float](repeatElement(Float(0), count: Int(width)))
            
            for i in 0 ..< displayData.count {
                let percent: Float = Float(i)/Float(width)
                let index: Int = Int(percent * Float(tableLength))
                if table != nil {
                    displayData[i] = (-(valTable[index] * middle * scalingFactor) + middle)
                }
            }
        }

        DispatchQueue.main.async { [unowned self] in
            self.setNeedsDisplay()
        }
    }
}

extension Waveview: CsoundBinding {
    func setup(_ csoundObj: CsoundObj!) {
        tableLoaded = false
        csObj = csoundObj
        fTableNumber = 1
    }
    
    // Update F-table values from Csound
    func updateValuesFromCsound() {
        if !tableLoaded {
            let cs = csObj.getCsound()
            tableLength = Int(csoundTableLength(cs, Int32(fTableNumber)))
            if tableLength > 0 {
                table = UnsafeMutablePointer<Float>.allocate(capacity: tableLength)
                csoundGetTable(cs, &table, Int32(fTableNumber))
                tableLoaded = true
            }
            
            DispatchQueue.global(qos: .background).async { [unowned self] in
                self.updateDisplayData()
            }
        }
    }
}
