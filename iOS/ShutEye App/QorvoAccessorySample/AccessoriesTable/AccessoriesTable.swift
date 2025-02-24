/*
 * @file      AccessoriesTable.swift
 *
 * @brief     Table where the Qorvo devices are listed. The cell for this table is
 *            implemented at SingleCell.swift.
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

import Foundation
import UIKit
import os.log

class AccessoriesTable: UITableView, UITableViewDelegate, UITableViewDataSource {
    
    let logger = os.Logger(subsystem: "com.qorvo.nibg", category: "AccessoriesTable")
    
    var tableDelegate: TableProtocol?
    
    override init(frame: CGRect, style: UITableView.Style) {
        super.init(frame: frame, style: style)
        
        // Set up the table view
        delegate = self
        dataSource = self
        
        // Register a cell class for reuse
        register(SingleCell.self, forCellReuseIdentifier: "SingleCell")
        
        rowHeight = ACCESSORY_TABLE_ROW_HEIGHT_CONSTRAINT
        separatorInset = .zero
        separatorStyle = .none
        tableFooterView = UIView()
        
        // Set up the parent view's constraints
        translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            heightAnchor.constraint(greaterThanOrEqualToConstant: ACCESSORY_TABLE_HEIGHT_CONSTRAINT),
            topAnchor.constraint(equalTo: topAnchor),
            leadingAnchor.constraint(equalTo: leadingAnchor),
            trailingAnchor.constraint(equalTo: trailingAnchor)
        ])
        
        separatorStyle = .none
        backgroundColor = .white
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func setCellAsset(_ deviceID: Int,_ newAsset: asset) {
        // Edit cell for this uniqueID
        for case let cell as SingleCell in self.visibleCells {
            if cell.uniqueID == deviceID {
                cell.selectAsset(newAsset)
            }
        }
    }
    
    func setCellColor(_ deviceID: Int,_ newColor: UIColor) {
        // Edit cell for this uniqueID
        for case let cell as SingleCell in self.visibleCells {
            if cell.uniqueID == deviceID {
                cell.accessoryButton.backgroundColor = newColor
            }
        }
    }
    
    func handleCell(_ index: Int,_ insert: Bool ) {
        self.beginUpdates()
        if (insert) {
            self.insertRows(at: [IndexPath(row: index, section: 0)], with: .automatic)
        }
        else {
            self.deleteRows(at: [IndexPath(row: index, section: 0)], with: .automatic)
        }
        self.endUpdates()
    }
    
    func updateCell(_ deviceID: Int,_ distance: Float) {
        for case let cell as SingleCell in self.visibleCells {
            if cell.uniqueID == deviceID {
                cell.distanceLabel.text = String(format: "meters".localized, distance)
            }
        }
    }
    
    // MARK: - UITableViewDataSource
    func numberOfSections(in tableView: UITableView) -> Int {
        // Only one section
        return 1
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        // Number of rows equals the number of accessories
        return qorvoDevices.count
    }
    
    func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
        let disconnect = UIContextualAction(style: .normal, title: "") { [self] (action, view, completion) in
            // Send the disconnection message to the device
            let cell = tableView.cellForRow(at: indexPath) as! SingleCell
            let deviceID = cell.uniqueID
            
            tableDelegate?.sendStopToDevice(deviceID)
            
            completion(true)
        }
        // Set the Contextual action parameters
        disconnect.image = UIImage(named: "trash_bin")
        disconnect.backgroundColor = .qorvoRed
        
        let swipeActions = UISwipeActionsConfiguration(actions: [disconnect])
        swipeActions.performsFirstActionWithFullSwipe = false
        
        return swipeActions
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "SingleCell", for: indexPath) as! SingleCell
        
        let qorvoDevice = qorvoDevices[indexPath.row]
        
        cell.uniqueID = (qorvoDevice?.bleUniqueID)!
        
        // Initialize the new cell assets
        cell.accessoryButton.tag = cell.uniqueID
        cell.accessoryButton.setTitle(qorvoDevice?.blePeripheralName, for: .normal)
        cell.accessoryButton.isEnabled = true
        
        cell.actionButton.tag = cell.uniqueID
        cell.actionButton.addTarget(self,
                                    action: #selector(buttonAction),
                                    for: .touchUpInside)
        cell.actionButton.isEnabled = true
        
        logger.info("New device included at row \(indexPath.row)")
        
        return cell
    }
    
    // MARK: - UITableViewDelegate
    func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
        return true
    }
    
    // MARK: - TableProtocol delegate wraper
    @objc func buttonAction(_ sender: UIButton) {
        tableDelegate?.buttonAction(sender)
    }
}
