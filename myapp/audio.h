//
//  test.h
//  myapp
//
//  Created by tianyang on 2020/10/27.
//  Copyright © 2020 tianyang. All rights reserved.
//

#ifndef test_h
#define test_h

#include <stdio.h>
#include <unistd.h>
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"

void set_status(int status);
void record_audio(void);

#endif /* test_h */
