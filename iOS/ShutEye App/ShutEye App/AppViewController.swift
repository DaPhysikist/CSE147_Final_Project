/*
 * @file      AppViewController.swift
 *
 * @brief     Main Application View Controller.
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
import SwiftUI
import NearbyInteraction
import os.log

// An example messaging protocol for communications between the app and the
// accessory. In your app, modify or extend this enumeration to your app's
// user experience and conform the accessory accordingly.
enum MessageId: UInt8 {
    // Messages from the accessory.
    case accessoryConfigurationData = 0x1
    case accessoryUwbDidStart = 0x2
    case accessoryUwbDidStop = 0x3
    case accessoryPaired = 0x4
    
    // Messages to the accessory.
    case initialize = 0xA
    case configureAndStart = 0xB
    case stop = 0xC
    
    // User defined/notification messages
    case getReserved = 0x20
    case setReserved = 0x21

    case iOSNotify = 0x2F
}

protocol ArrowProtocol: AnyObject {
    func switch3DArrow()
}

protocol TableProtocol: AnyObject {
    func buttonAction(_ sender: UIButton)
    func sendStopToDevice(_ deviceID: Int)
}

class AppViewController: UIViewController, TableProtocol {
   
    @IBOutlet weak var mainStackView: UIStackView!
    
    // All info Views
    let separatorView = SeparatorView(fieldTitle: "Accessories near in background mode")
    let accessoriesTable = AccessoriesTable()
    
    var dataChannel = DataCommunicationChannel()
    var notifier = NotificationManager.instance
    
    var configuration: NINearbyAccessoryConfiguration?
    // Dictionary to associate each NI Session to the qorvoDevice using the uniqueID
    var referenceDict = [Int:NISession]()
    // A mapping from a discovery token to a name.
    var accessoryMap = [NIDiscoveryToken: String]()
    // Settings from View are initialised by the main controller, wich use these settings
    let savedSettings = UserDefaults.standard
    
    let logger = os.Logger(subsystem: "com.qorvo.nibg", category: "QorvoDemoViewController")
    
    let energyChartButton = UIButton(type: .system)       //Create button object for viewing energy charts
        
    func setupEnergyChartButton() {                                           //Function to add the button with styling to the view in the app GUI
        energyChartButton.setTitle("View Energy Charts", for: .normal)
        energyChartButton.backgroundColor = .systemBlue
        energyChartButton.setTitleColor(.white, for: .normal)
        energyChartButton.layer.cornerRadius = 8
        energyChartButton.addTarget(self, action: #selector(showEnergyCharts), for: .touchUpInside)   //Add the button 'click' action, calls the function to open the chart view

        energyChartButton.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(energyChartButton)

        NSLayoutConstraint.activate([
            energyChartButton.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            energyChartButton.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -20),
            energyChartButton.widthAnchor.constraint(equalToConstant: 200),
            energyChartButton.heightAnchor.constraint(equalToConstant: 50)
        ])
    }

    @objc func showEnergyCharts() {
        let chartView = UIHostingController(rootView: BarChartView())            //Switch to the ChartView when button is tapped
        navigationController?.pushViewController(chartView, animated: true)
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupEnergyChartButton()    //Calls the function to load the button in with its styling to the app GUI
        
        // Notification from Settings View Controller
        NotificationCenter.default.addObserver(self,
                                               selector: #selector(clearKnownDevices),
                                               name: Notification.Name("clearKnownDevices"),
                                               object: nil)
        NotificationCenter.default.addObserver(self,
                                               selector: #selector(settingsNotification),
                                               name: Notification.Name("settingsNotification"),
                                               object: nil)
        
        notifier.requestAuthorization()
        
        // Initialise Settings
        if let state = savedSettings.object(forKey: "settingsNotification") as? Bool {
            appSettings.pushNotificationEnabled = state
        }
        
        // Set delegate to allow "accessoriesTable" to use TableProtocol
        accessoriesTable.tableDelegate = self
        
        mainStackView.insertArrangedSubview(separatorView, at: 0)
        mainStackView.insertArrangedSubview(accessoriesTable, at: 1)
        mainStackView.overrideUserInterfaceStyle = .light
        
        // Prepare the data communication channel.
        dataChannel.accessorySynchHandler = accessorySynch
        dataChannel.accessoryPairHandler = accessoryPaired
        dataChannel.accessoryConnectedHandler = accessoryConnected
        dataChannel.accessoryDisconnectedHandler = accessoryDisconnected
        dataChannel.accessoryDataHandler = accessorySharedData
        dataChannel.start()
        
        logger.info("Scanning for accessories")
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
        .lightContent
    }
    
    @objc func clearKnownDevices(notification: NSNotification) {
        dataChannel.clearKnownDevices()
    }
    
    @objc func settingsNotification(notification: NSNotification) {
        savedSettings.set(appSettings.pushNotificationEnabled, forKey: "settingsNotification")
    }
    
    @IBAction func buttonAction(_ sender: UIButton) {
        let deviceID = sender.tag
        
        reqConnectionByID(deviceID)
        logger.info("Action Button pressed for device \(deviceID)")
    }
    
    func reqConnectionByID(_ deviceID: Int) {
        // Get qorvo device that's match with sender's tag
        if let qorvoDevice = dataChannel.getDeviceFromUniqueID(deviceID) {
            // Connect to the accessory
            if qorvoDevice.blePeripheralStatus == statusDiscovered {
                logger.info("Connecting to Accessory")
                connectToAccessory(deviceID)
            }
            else {
                return
            }

            // Edit cell for this sender
            accessoriesTable.setCellAsset(deviceID, .connecting)
        }
    }
    
    // MARK: - Data Channel functions and callbacks
    func sendStopToDevice(_ deviceID: Int) {
        let qorvoDevice = dataChannel.getDeviceFromUniqueID(deviceID)
        
        if qorvoDevice?.blePeripheralStatus != statusDiscovered {
            sendDataToAccessory(Data([MessageId.stop.rawValue]), deviceID)
        }
    }
    
    // MARK: - Data channel methods
    func accessorySharedData(data: Data, accessoryName: String, deviceID: Int) {
        // The accessory begins each message with an identifier byte.
        // Ensure the message length is within a valid range.
        if data.count < 1 {
            logger.debug("Accessory shared data length was less than 1.")
            return
        }
        
        // Assign the first byte which is the message identifier.
        guard let messageId = MessageId(rawValue: data.first!) else {
            fatalError("\(data.first!) is not a valid MessageId.")
        }
        
        // Handle the data portion of the message based on the message identifier.
        switch messageId {
        case .accessoryConfigurationData:
            // Access the message data by skipping the message identifier.
            assert(data.count > 1)
            let message = data.advanced(by: 1)
            setupAccessory(message, name: accessoryName, deviceID: deviceID)
        case .accessoryUwbDidStart:
            handleAccessoryUwbDidStart(deviceID)
        case .accessoryUwbDidStop:
            handleAccessoryUwbDidStop(deviceID)
        case .accessoryPaired:
            accessoryPaired(data: data, deviceID: deviceID)
        case .configureAndStart:
            fatalError("Accessory should not send 'configureAndStart'.")
        case .initialize:
            fatalError("Accessory should not send 'initialize'.")
        case .stop:
            fatalError("Accessory should not send 'stop'.")
        // User defined/notification messages
        case .getReserved:
            logger.debug("Get not implemented in this version")
        case .setReserved:
            logger.debug("Set not implemented in this version")
        case .iOSNotify:
            if appSettings.pushNotificationEnabled! {
                let qorvoDevice = dataChannel.getDeviceFromUniqueID(deviceID)
                
                if let message = String(bytes: data.advanced(by: 3), encoding: .utf8) {
                    notifier.setNotification(deviceName: qorvoDevice!.blePeripheralName,
                                             deviceMessage: message)
                }
            }
        }
    }
    
    func accessorySynch(_ index: Int,_ deviceID: Int, insert: Bool ) {
        accessoriesTable.handleCell(index, insert)
        
        if insert, dataChannel.checkPairedDevice(deviceID) {
            reqConnectionByID(deviceID)
        }
    }
    
    func accessoryUpdate() {
        // Update cells based on their status
        qorvoDevices.forEach { (qorvoDevice) in
            if qorvoDevice?.blePeripheralStatus == statusDiscovered {
                accessoriesTable.setCellAsset(qorvoDevice!.bleUniqueID,
                                              .actionButton)
            }
        }
    }
    
    func accessoryConnected(deviceID: Int) {
        pairToAccessory(deviceID)
    }
    
    func accessoryPaired(data: Data, deviceID: Int) {
        // Add device if not added yet
        if !(dataChannel.checkPairedDevice(deviceID)) {
            dataChannel.includePairedDevice(deviceID, "test")
        }
        
        // Create a NISession for the new device
        referenceDict[deviceID] = NISession()
        referenceDict[deviceID]?.delegate = self
        
        logger.info("Requesting configuration data from accessory")
        let msg = Data([MessageId.initialize.rawValue])
        
        sendDataToAccessory(msg, deviceID)
    }
    
    func accessoryDisconnected(deviceID: Int) {
        referenceDict[deviceID]?.invalidate()
        // Remove the NI Session and Location values related to the device ID
        referenceDict.removeValue(forKey: deviceID)
        
        accessoryUpdate()
        
        // Update device list and take other actions depending on the amount of devices
        let deviceCount = qorvoDevices.count
    }
    
    // MARK: - Accessory messages handling
    func setupAccessory(_ configData: Data, name: String, deviceID: Int) {
        logger.info("Received configuration data from '\(name)'. Running session.")
        
        // Get peerIdentifier from the connected device
        let peerDevice = dataChannel.getDeviceFromUniqueID(deviceID)
        let peerIdentifier = peerDevice!.blePeripheral.identifier

        do {
            configuration = try NINearbyAccessoryConfiguration(accessoryData: configData,
                                                               bluetoothPeerIdentifier: peerIdentifier)
        } catch {
            // Stop and display the issue because the incoming data is invalid.
            // In your app, debug the accessory data to ensure an expected
            // format.
            logger.info("Failed to create NINearbyAccessoryConfiguration for '\(name)'. Error: \(error)")
            return
        }
        
        // Cache the token to correlate updates with this accessory.
        cacheToken(configuration!.accessoryDiscoveryToken, accessoryName: name)
        
        referenceDict[deviceID]?.run(configuration!)
        logger.info("Accessory Background Session configured.")
    }
    
    func handleAccessoryUwbDidStart(_ deviceID: Int) {
        logger.info("Accessory Session started.")
        
        // Update the device Status
        if let startedDevice = dataChannel.getDeviceFromUniqueID(deviceID) {
            startedDevice.blePeripheralStatus = statusRanging
        }
        
        accessoriesTable.setCellAsset(deviceID, .miniLocation)
    }
    
    func handleAccessoryUwbDidStop(_ deviceID: Int) {
        logger.info("Accessory Session stopped.")
        
        // Disconnect from device
        disconnectFromAccessory(deviceID)
    }
    
    func updateMiniFields(_ deviceID: Int) {
        
        let qorvoDevice = dataChannel.getDeviceFromUniqueID(deviceID)
        if qorvoDevice == nil { return }
        
        // Get updated location values
        let distance  = qorvoDevice?.uwbDistance

        // Update the "accessoriesTable" cell with the given values
        accessoriesTable.updateCell(deviceID, distance!)
    }
    
}

// MARK: - `NISessionDelegate`.
extension AppViewController: NISessionDelegate {

    func session(_ session: NISession, didGenerateShareableConfigurationData shareableConfigurationData: Data, for object: NINearbyObject) {
        guard object.discoveryToken == configuration?.accessoryDiscoveryToken else { return }
        
        // Prepare to send a message to the accessory.
        var msg = Data([MessageId.configureAndStart.rawValue])
        msg.append(shareableConfigurationData)
        
        let str = msg.map { String(format: "0x%02x, ", $0) }.joined()
        logger.info("Sending shareable configuration bytes: \(str)")
        
        // Send the message to the correspondent accessory.
        sendDataToAccessory(msg, deviceIDFromSession(session))
        logger.info("Sent shareable configuration data")
    }
    
    func session(_ session: NISession, didUpdate nearbyObjects: [NINearbyObject]) {
        guard let accessory = nearbyObjects.first else { return }
        guard let distance  = accessory.distance else { return }
        
        let deviceID = deviceIDFromSession(session)
        //logger.info(NISession.deviceCapabilities)
    
        if let updatedDevice = dataChannel.getDeviceFromUniqueID(deviceID) {
            // set updated values
            updatedDevice.uwbDistance = distance
            updatedDevice.blePeripheralStatus = statusRanging
        }
        
        updateMiniFields(deviceID)
    }
    
    func session(_ session: NISession, didRemove nearbyObjects: [NINearbyObject], reason: NINearbyObject.RemovalReason) {
        // Retry the session only if the peer timed out.
        guard reason == .timeout else { return }
        logger.info("Session timed out")
        
        // The session runs with one accessory.
        guard let accessory = nearbyObjects.first else { return }
        
        // Clear the app's accessory state.
        accessoryMap.removeValue(forKey: accessory.discoveryToken)
        
        // Get the deviceID associated to the NISession
        let deviceID = deviceIDFromSession(session)
        
        // Consult helper function to decide whether or not to retry.
        if shouldRetry(deviceID) {
            sendDataToAccessory(Data([MessageId.stop.rawValue]), deviceID)
            sendDataToAccessory(Data([MessageId.initialize.rawValue]), deviceID)
        }
    }
    
    func sessionWasSuspended(_ session: NISession) {
        logger.info("Session was suspended")
        let msg = Data([MessageId.stop.rawValue])
        
        sendDataToAccessory(msg, deviceIDFromSession(session))
    }
    
    func sessionSuspensionEnded(_ session: NISession) {
        logger.info("Session suspension ended")
        // When suspension ends, restart the configuration procedure with the accessory.
        let msg = Data([MessageId.initialize.rawValue])
        
        sendDataToAccessory(msg, deviceIDFromSession(session))
    }
    
    func session(_ session: NISession, didInvalidateWith error: Error) {
        let deviceID = deviceIDFromSession(session)
        
        switch error {
        case NIError.invalidConfiguration:
            // Debug the accessory data to ensure an expected format.
            logger.info("The accessory configuration data is invalid. Please debug it and try again.")
        case NIError.userDidNotAllow:
            handleUserDidNotAllow()
        default:
            handleSessionInvalidation(deviceID)
        }
    }
}

// MARK: - Helpers.
extension AppViewController {
    
    func pairToAccessory(_ deviceID: Int) {
         do {
             try dataChannel.pairPeripheral(deviceID)
         } catch {
             logger.info("Failed to pair to accessory: \(error)")
         }
    }
    
    func connectToAccessory(_ deviceID: Int) {
         do {
             try dataChannel.connectPeripheral(deviceID)
         } catch {
             logger.info("Failed to connect to accessory: \(error)")
         }
    }
    
    func disconnectFromAccessory(_ deviceID: Int) {
         do {
             try dataChannel.disconnectPeripheral(deviceID)
         } catch {
             logger.info("Failed to disconnect from accessory: \(error)")
         }
     }
    
    func sendDataToAccessory(_ data: Data,_ deviceID: Int) {
         do {
             try dataChannel.sendData(data, deviceID)
         } catch {
             logger.info("Failed to send data to accessory: \(error)")
         }
     }
    
    func handleSessionInvalidation(_ deviceID: Int) {
        logger.info("Session invalidated. Restarting.")
        // Ask the accessory to stop.
        sendDataToAccessory(Data([MessageId.stop.rawValue]), deviceID)

        // Replace the invalidated session with a new one.
        referenceDict[deviceID] = NISession()
        referenceDict[deviceID]?.delegate = self

        // Ask the accessory to stop.
        sendDataToAccessory(Data([MessageId.initialize.rawValue]), deviceID)
    }
    
    func shouldRetry(_ deviceID: Int) -> Bool {
        // Need to use the dictionary here, to know which device failed and check its connection state
        let qorvoDevice = dataChannel.getDeviceFromUniqueID(deviceID)
        
        if qorvoDevice?.blePeripheralStatus != statusDiscovered {
            return true
        }
        
        return false
    }
    
    func deviceIDFromSession(_ session: NISession)-> Int {
        var deviceID = -1
        
        for (key, value) in referenceDict {
            if value == session {
                deviceID = key
            }
        }
        
        return deviceID
    }
    
    func cacheToken(_ token: NIDiscoveryToken, accessoryName: String) {
        accessoryMap[token] = accessoryName
    }
    
    func handleUserDidNotAllow() {
        // Beginning in iOS 15, persistent access state in Settings.
        logger.info("Nearby Interactions access required. You can change access for NIAccessory in Settings.")
        
        // Create an alert to request the user go to Settings.
        let accessAlert = UIAlertController(title: "AccessRequired".localized,
                                            message: "NIAccessRequired.message".localized,
                                            preferredStyle: .alert)
        accessAlert.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
        accessAlert.addAction(UIAlertAction(title: "GoSettings".localized, style: .default, handler: {_ in
            // Navigate the user to the app's settings.
            if let settingsURL = URL(string: UIApplication.openSettingsURLString) {
                UIApplication.shared.open(settingsURL, options: [:], completionHandler: nil)
            }
        }))

        // Preset the access alert.
        present(accessAlert, animated: true, completion: nil)
    }
}

// MARK: - Utils.
extension String {
    var localized: String {
        return NSLocalizedString(self, tableName: nil, bundle: Bundle.main, value: "", comment: "")
    }
    var localizedUppercase: String {
        return NSLocalizedString(self, tableName: nil, bundle: Bundle.main, value: "", comment: "").uppercased()
    }
}

extension UIViewController {
    func presentSwiftUIView<Content: View>(_ swiftUIView: Content, title: String) {
        let hostingController = UIHostingController(rootView: swiftUIView)
        hostingController.title = title
        navigationController?.pushViewController(hostingController, animated: true)
    }
}
