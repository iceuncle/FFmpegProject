//
//  ViewController.swift
//  myapp
//
//  Created by tianyang on 2020/10/27.
//  Copyright © 2020 tianyang. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {

    var recStatus: Bool = false;
    let btn = NSButton.init(title: "", target: self, action: #selector(myFunc))
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.view.setFrameSize(NSSize(width: 320, height: 240))
        btn.title = "开始录制"
        btn.frame = NSRect(x: 320/2-60, y: 240/2-15, width: 120, height: 30)
        btn.bezelStyle = .rounded
        btn.setButtonType(.pushOnPushOff)
        self.view.addSubview(btn)
        
    }
    
    @objc func myFunc() {
        recStatus = !recStatus;
        if recStatus {
            let thread: Thread? = Thread.init(target: self, selector: #selector(self.recAudio), object: nil)
            thread?.start()
            self.btn.title = "停止录制"
        } else {
            set_status(0);
            self.btn.title = "开始录制"
        }
    }
    
    @objc func recAudio() {
        record_audio();
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

