/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "../../Map.h"
#include <vts-browser/statistics.hpp>

#import "StatisticsViewController.h"


@interface StatisticsViewController ()
{
    NSTimer* timer;
}

@property (weak, nonatomic) IBOutlet UILabel *statCpuMem;
@property (weak, nonatomic) IBOutlet UILabel *statGpuMem;
@property (weak, nonatomic) IBOutlet UILabel *statActiveResources;
@property (weak, nonatomic) IBOutlet UILabel *statPreparingResources;
@property (weak, nonatomic) IBOutlet UILabel *statDownloading;
@property (weak, nonatomic) IBOutlet UILabel *statMetaUpdates;
@property (weak, nonatomic) IBOutlet UILabel *statDrawUpdates;

@end


@implementation StatisticsViewController

- (void)updateView
{
    _statCpuMem.text = [NSString stringWithFormat:@"%llu MB", map->statistics().currentRamMemUse / 1024 / 1024];
    _statGpuMem.text = [NSString stringWithFormat:@"%llu MB", map->statistics().currentGpuMemUse / 1024 / 1024];
    _statActiveResources.text = [NSString stringWithFormat:@"%u", map->statistics().currentResources];
    _statPreparingResources.text = [NSString stringWithFormat:@"%u", map->statistics().currentResourcePreparing];
    _statDownloading.text = [NSString stringWithFormat:@"%u", map->statistics().currentResourceDownloads];
    _statMetaUpdates.text = [NSString stringWithFormat:@"%u", map->statistics().currentNodeMetaUpdates];
    _statDrawUpdates.text = [NSString stringWithFormat:@"%u", map->statistics().currentNodeDrawsUpdates];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self updateView];
    mapTimerStart(self, @selector(timerTick));
}

- (void)timerTick
{
    [self updateView];
}

@end
