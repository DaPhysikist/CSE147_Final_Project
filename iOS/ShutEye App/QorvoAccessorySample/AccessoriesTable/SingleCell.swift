/*
 * @file      SingleCell.swift
 *
 * @brief     Class to handle components inside Table's View Prototype Cell
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

enum asset: String {
    // Index for each label.
    case actionButton = "Button to Connect"
    case connecting   = "Animated Icon"
    case miniLocation = "Panel with TWR info"
}

class SingleCell: UITableViewCell {
    
    let accessoryButton: UIButton
    let miniLocation: UIView
    let actionButton: UIButton
    let connecting: UIImageView
    let bottomBar: UIImageView
    
    let pipe: UIImageView
    let distanceLabel: UITextField
    
    // Used to animate scanning images
    var imageLoading = [UIImage]()
    var uniqueID: Int = 0
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        
        accessoryButton = UIButton()
        accessoryButton.titleLabel?.font = .dinNextRegular_s
        accessoryButton.setTitleColor(.black, for: .normal)
        accessoryButton.contentHorizontalAlignment = .left
        accessoryButton.titleEdgeInsets = UIEdgeInsets(top: 0, left: 20, bottom: 0, right: 0)
        //accessoryButton.configuration?.titlePadding = 20
        accessoryButton.translatesAutoresizingMaskIntoConstraints = false
        
        miniLocation = UIView()
        miniLocation.translatesAutoresizingMaskIntoConstraints = false
        
        pipe = UIImageView(image: UIImage(named: "subheading"))
        pipe.contentMode = .scaleAspectFit
        pipe.translatesAutoresizingMaskIntoConstraints = false
        
        distanceLabel = UITextField(frame: .zero)
        distanceLabel.translatesAutoresizingMaskIntoConstraints = false
        distanceLabel.font = .dinNextMedium_s
        distanceLabel.textAlignment = .right
        distanceLabel.textColor = .black
        distanceLabel.text = "StartMeters".localized
        
        actionButton = UIButton()
        actionButton.titleLabel?.font = .dinNextMedium_m
        actionButton.setTitleColor(.qorvoBlue, for: .normal)
        actionButton.setTitle("Connect".localized, for: .normal)
        actionButton.contentHorizontalAlignment = .right
        actionButton.titleEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 20)
        //actionButton.configuration?.titlePadding = 20
        actionButton.translatesAutoresizingMaskIntoConstraints = false
        
        connecting = UIImageView()
        connecting.contentMode = .scaleAspectFit
        connecting.translatesAutoresizingMaskIntoConstraints = false
        
        bottomBar = UIImageView(image: UIImage(named: "bar"))
        bottomBar.contentMode = .scaleAspectFit
        bottomBar.translatesAutoresizingMaskIntoConstraints = false
        
        miniLocation.addSubview(distanceLabel)
        miniLocation.addSubview(pipe)
        
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        
        contentView.backgroundColor = .white
        contentView.translatesAutoresizingMaskIntoConstraints = false
        
        contentView.addSubview(accessoryButton)
        contentView.addSubview(miniLocation)
        contentView.addSubview(actionButton)
        contentView.addSubview(connecting)
        contentView.addSubview(bottomBar)
        
        // Start the Activity Indicators
        let imageSmall = UIImage(named: "spinner_small")!
        for i in 0...24 {
            imageLoading.append(imageSmall.rotate(radians: Float(i) * .pi / 12)!)
        }
        connecting.animationImages = imageLoading
        connecting.animationDuration = 1
        
        // Set up the stack view's constraints
        NSLayoutConstraint.activate([
            contentView.topAnchor.constraint(equalTo: topAnchor),
            contentView.bottomAnchor.constraint(equalTo: bottomAnchor),
            contentView.leadingAnchor.constraint(equalTo: leadingAnchor),
            contentView.trailingAnchor.constraint(equalTo: trailingAnchor),
            
            accessoryButton.topAnchor.constraint(equalTo: topAnchor),
            accessoryButton.bottomAnchor.constraint(equalTo: bottomAnchor),
            accessoryButton.leadingAnchor.constraint(equalTo: leadingAnchor),
            accessoryButton.trailingAnchor.constraint(equalTo: trailingAnchor),
            
            actionButton.topAnchor.constraint(equalTo: topAnchor),
            actionButton.bottomAnchor.constraint(equalTo: bottomAnchor),
            actionButton.trailingAnchor.constraint(equalTo: trailingAnchor),
            actionButton.widthAnchor.constraint(equalToConstant: ACTION_BUTTON_WIDTH_CONSTRAINT),
            
            connecting.heightAnchor.constraint(equalToConstant: CONNECTING_SIDE_CONSTRAINT),
            connecting.widthAnchor.constraint(equalToConstant: CONNECTING_SIDE_CONSTRAINT),
            connecting.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -20),
            connecting.centerYAnchor.constraint(equalTo: centerYAnchor),
            
            bottomBar.heightAnchor.constraint(equalToConstant: BOTTOM_BAR_HEIGHT_CONSTRAINT),
            bottomBar.widthAnchor.constraint(equalToConstant: BOTTOM_BAR_WIDTH_CONSTRAINT),
            bottomBar.bottomAnchor.constraint(equalTo: bottomAnchor, constant: -1),
            bottomBar.centerXAnchor.constraint(equalTo: centerXAnchor),
            
            // miniLocation view is where the location asstes are nested
            miniLocation.topAnchor.constraint(equalTo: topAnchor),
            miniLocation.bottomAnchor.constraint(equalTo: bottomAnchor),
            miniLocation.trailingAnchor.constraint(equalTo: trailingAnchor),
            miniLocation.widthAnchor.constraint(equalToConstant: MINI_LOCATION_WIDTH_CONSTRAINT),
            
            distanceLabel.widthAnchor.constraint(equalToConstant: DISTANCE_LABEL_WIDTH_CONSTRAINT),
            distanceLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -20),
            distanceLabel.centerYAnchor.constraint(equalTo: centerYAnchor),
            
            pipe.heightAnchor.constraint(equalToConstant: PIPE_SIDE_CONSTRAINT),
            pipe.widthAnchor.constraint(equalToConstant: PIPE_SIDE_CONSTRAINT),
            pipe.trailingAnchor.constraint(equalTo: distanceLabel.leadingAnchor),
            pipe.centerYAnchor.constraint(equalTo: centerYAnchor),
        ])
        
        backgroundColor = .white
        
        selectAsset(.actionButton)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    func selectAsset(_ asset: asset) {
        switch asset {
        case .actionButton:
            miniLocation.isHidden = true
            actionButton.isHidden = false
            connecting.isHidden   = true
            connecting.stopAnimating()
        case .connecting:
            miniLocation.isHidden = true
            actionButton.isHidden = true
            connecting.isHidden   = false
            connecting.startAnimating()
        case .miniLocation:
            miniLocation.isHidden = false
            actionButton.isHidden = true
            connecting.isHidden   = true
            connecting.stopAnimating()
        }
    }
}
