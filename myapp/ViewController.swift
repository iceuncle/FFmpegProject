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
    let audioBtn = NSButton.init(title: "", target: self, action: #selector(audioFunc))
    
    var recVideoStatus: Bool = false;
    let videoBtn = NSButton.init(title: "", target: self, action: #selector(videoFunc))
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.view.setFrameSize(NSSize(width: 320, height: 240))
        audioBtn.title = "开始录制音频"
        audioBtn.frame = NSRect(x: 320/2-60, y: 240/2+15, width: 120, height: 30)
        audioBtn.bezelStyle = .rounded
        audioBtn.setButtonType(.pushOnPushOff)
        self.view.addSubview(audioBtn)
        
        videoBtn.title = "开始录制视频"
        videoBtn.frame = NSRect(x: 320/2-60, y: 240/2-35, width: 120, height: 30)
        videoBtn.bezelStyle = .rounded
        videoBtn.setButtonType(.pushOnPushOff)
        self.view.addSubview(videoBtn)
        
        
    }
    
    @objc func audioFunc() {
        recStatus = !recStatus;
        if recStatus {
            let thread: Thread? = Thread.init(target: self, selector: #selector(self.recAudio), object: nil)
            thread?.start()
            self.audioBtn.title = "停止录制音频"
        } else {
            set_status(0);
            self.audioBtn.title = "开始录制音频"
        }
    }
    
    @objc func recAudio() {
        record_audio();
    }
    
    @objc func videoFunc() {
       recVideoStatus = !recVideoStatus;
       if recVideoStatus {
           let thread: Thread? = Thread.init(target: self, selector: #selector(self.recVideo), object: nil)
           thread?.start()
           self.videoBtn.title = "停止录制视频"
       } else {
           set_video_status(0);
           self.videoBtn.title = "开始录制视频"
       }
    }

    @objc func recVideo() {
       record_video();
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

