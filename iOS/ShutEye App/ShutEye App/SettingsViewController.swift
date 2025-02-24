/*
 * @file      SettingsViewController.swift
 *
 * @brief     A View Controller to view/set the application parameters.
 *
 * @author    Decawave Applications
 *
 * @attention Copyright (c) 2021 - 2022, Qorvo US, Inc.
 * All rights reserved
 * Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this
 *  list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 3. You may only use this software, with or without any modification, with an
 *  integrated circuit developed by Qorvo US, Inc. or any of its affiliates
 *  (collectively, "Qorvo"), or any module that contains such integrated circuit.
 * 4. You may not reverse engineer, disassemble, decompile, decode, adapt, or
 *  otherwise attempt to derive or gain access to the source code to any software
 *  distributed under this license in binary or object code form, in whole or in
 *  part.
 * 5. You may not use any Qorvo name, trademarks, service marks, trade dress,
 *  logos, trade names, or other symbols or insignia identifying the source of
 *  Qorvo's products or services, or the names of any of Qorvo's developers to
 *  endorse or promote products derived from this software without specific prior
 *  written permission from Qorvo US, Inc. You must not call products derived from
 *  this software "Qorvo", you must not have "Qorvo" appear in their name, without
 *  the prior permission from Qorvo US, Inc.
 * 6. Qorvo may publish revised or new version of this license from time to time.
 *  No one other than Qorvo US, Inc. has the right to modify the terms applicable
 *  to the software provided under this license.
 * THIS SOFTWARE IS PROVIDED BY QORVO US, INC. "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. NEITHER
 *  QORVO, NOR ANY PERSON ASSOCIATED WITH QORVO MAKES ANY WARRANTY OR
 *  REPRESENTATION WITH RESPECT TO THE COMPLETENESS, SECURITY, RELIABILITY, OR
 *  ACCURACY OF THE SOFTWARE, THAT IT IS ERROR FREE OR THAT ANY DEFECTS WILL BE
 *  CORRECTED, OR THAT THE SOFTWARE WILL OTHERWISE MEET YOUR NEEDS OR EXPECTATIONS.
 * IN NO EVENT SHALL QORVO OR ANYBODY ASSOCIATED WITH QORVO BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *
 */

import UIKit
import os
import NearbyInteraction

public struct Settings {
    var pushNotificationEnabled: Bool?
    
    init() {
        pushNotificationEnabled = true;
    }
}

extension UIImage {
    enum AssetIdentifier: String {
        case SwitchOn = "switch_on.svg"
        case SwitchOff = "switch_off.svg"
    }
    convenience init(assetIdentifier: AssetIdentifier) {
        self.init(named: assetIdentifier.rawValue)!
    }
}

public var appSettings: Settings = Settings.init()

// UIButton extension which enables the caller to duplicate a UIButton
extension UIStackView {
    func copyStackView() -> UIStackView? {
        
        // Attempt to duplicate button by archiving and unarchiving the original UIButton
        guard let archived = try? NSKeyedArchiver.archivedData(withRootObject: self,
                                                               requiringSecureCoding: false)
        else {
            fatalError("archivedData failed")
        }
        
        guard let copy = try? NSKeyedUnarchiver.unarchiveTopLevelObjectWithData(archived) as? UIStackView
        else {
            fatalError("unarchivedData failed")
        }
        
        return copy
    }
}

class SettingsViewController: UIViewController {
    
    @IBOutlet weak var enableNotification: UIButton!
    @IBOutlet weak var accessorySample: UIStackView!
    @IBOutlet weak var accessoriesList: UIStackView!
    @IBOutlet weak var scanning: UIImageView!
    @IBOutlet weak var clearKnownDevices: UIButton!
    
    // Dictionary to co-relate BLE Device Unique ID with its UIStackViews hashValues
    var referenceDict = [Int:UIStackView]()
    var animationCounter: Int = 0
    
    let logger = os.Logger(subsystem: "com.qorvo.nibg", category: "Settings")
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Initialize switches
        if appSettings.pushNotificationEnabled! {
            enableNotification.setImage(UIImage(assetIdentifier: .SwitchOn), for: .normal)
        }
        else {
            enableNotification.setImage(UIImage(assetIdentifier: .SwitchOff), for: .normal)
        }
        
        updateDeviceList()
        
        // Start the Activity Indicator
        var imageArray = [UIImage]()
        let image = UIImage(named: "spinner.svg")!
        for i in 0...24 {
            imageArray.append(image.rotate(radians: Float(i) * .pi / 12)!)
        }
        scanning.animationImages = imageArray
        scanning.animationDuration = 1
        scanning.startAnimating()
        
        // Set animation for "Clear Known Devices" button
        var imageArray_small = [UIImage]()
        let imageSmall = UIImage(named: "spinner_small")!
        for i in 0...24 {
            imageArray_small.append(imageSmall.rotate(radians: Float(i) * .pi / 12)!)
        }
        clearKnownDevices.imageView?.animationImages = imageArray_small
        clearKnownDevices.imageView?.animationDuration = 1
        clearKnownDevices.imageView?.isHidden  = true
        
        // Initialises the Timer used for update the device list
        _ = Timer.scheduledTimer(timeInterval: 0.5,
                                 target: self,
                                 selector: #selector(timerHandler),
                                 userInfo: nil,
                                 repeats: true)
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
            .lightContent
    }
    
    @IBAction func backToMain(_ sender: Any) {
        navigationController?.popViewController(animated: true)
    }
    
    @IBAction func toggleNotification(_ sender: Any) {
        if appSettings.pushNotificationEnabled! {
            enableNotification.setImage(UIImage(assetIdentifier: .SwitchOff), for: .normal)
            appSettings.pushNotificationEnabled = false
        }
        else {
            enableNotification.setImage(UIImage(assetIdentifier: .SwitchOn), for: .normal)
            appSettings.pushNotificationEnabled = true
        }
    }
    
    @IBAction func clearKnownDevices(_ sender: Any) {
        // Remove devices from the device list here
        NotificationCenter.default.post(name: Notification.Name("clearKnownDevices"), object: nil)
        
        clearKnownDevices.setTitle("", for: .normal)
        clearKnownDevices.imageView?.isHidden = false
        clearKnownDevices.imageView?.startAnimating()
        
        animationCounter = 4
    }
    
    @objc func timerHandler() {
        updateDeviceList()
        
        if animationCounter > 0 {
            animationCounter -= animationCounter
        } else {
            clearKnownDevices.setTitle("Clear Known Devices", for: .normal)
            clearKnownDevices.imageView?.stopAnimating()
            clearKnownDevices.imageView?.isHidden = true
        }
    }
    
    func updateDeviceList() {
        var removeFromDict: Bool
        
        // Add new devices, if any
        qorvoDevices.forEach { (qorvoDevice) in
            // Check if the device is already included
            if referenceDict[(qorvoDevice?.bleUniqueID)!] == nil {
                // Create a new StackView and add it to the main StackView
                let newDevice: UIStackView = accessorySample.copyStackView()!
                
                if let device = newDevice.arrangedSubviews.first as? UILabel {
                    device.text = qorvoDevice?.blePeripheralName
                }
                if let status = newDevice.arrangedSubviews.last as? UILabel {
                    status.text = qorvoDevice?.blePeripheralStatus
                }
                
                accessoriesList.addArrangedSubview(newDevice)
                UIView.animate(withDuration: 0.2) {
                    newDevice.isHidden =  false
                }

                // Add the new entry to the dictionary
                referenceDict[(qorvoDevice?.bleUniqueID)!] = newDevice
            }
        }
        
        // Remove devices, if they are no longer included
        for (key, value) in referenceDict {
            removeFromDict = true

            qorvoDevices.forEach { (qorvoDevice) in
                if key == qorvoDevice?.bleUniqueID {
                    removeFromDict = false
                }
            }

            if removeFromDict {
                referenceDict.removeValue(forKey: key)
                value.removeFromSuperview()
            }
        }
    }
}
