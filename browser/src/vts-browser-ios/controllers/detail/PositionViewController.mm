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

#import "PositionViewController.h"


@interface PositionViewController ()

@property (weak, nonatomic) IBOutlet UILabel *labX;
@property (weak, nonatomic) IBOutlet UILabel *labY;
@property (weak, nonatomic) IBOutlet UILabel *labZ;
@property (weak, nonatomic) IBOutlet UILabel *labYaw;
@property (weak, nonatomic) IBOutlet UILabel *labPitch;
@property (weak, nonatomic) IBOutlet UILabel *labRoll;
@property (weak, nonatomic) IBOutlet UILabel *labViewExtent;
@property (weak, nonatomic) IBOutlet UILabel *labFov;

@end


@implementation PositionViewController

- (void)updateView
{
    double tmp[3];
    // position
    navigation->getPoint(tmp);
    _labX.text = [NSString stringWithFormat:@"%f", tmp[0]];
    _labY.text = [NSString stringWithFormat:@"%f", tmp[1]];
    _labZ.text = [NSString stringWithFormat:@"%f", tmp[2]];
    // rotation
    navigation->getRotation(tmp);
    _labYaw.text = [NSString stringWithFormat:@"%f", tmp[0]];
    _labPitch.text = [NSString stringWithFormat:@"%f", tmp[1]];
    _labRoll.text = [NSString stringWithFormat:@"%f", tmp[2]];
    // other
    _labViewExtent.text = [NSString stringWithFormat:@"%f", navigation->getViewExtent()];
    _labFov.text = [NSString stringWithFormat:@"%f", navigation->getFov()];
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
