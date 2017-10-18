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
#include <vts-browser/options.hpp>

#import "OptionsViewController.h"


@interface OptionsViewController ()

@property (weak, nonatomic) IBOutlet UISegmentedControl *optTraversal;
@property (weak, nonatomic) IBOutlet UISlider *optQualityDegrad;
@property (weak, nonatomic) IBOutlet UISwitch *optAtmosphere;
@property (weak, nonatomic) IBOutlet UISwitch *optTouchAreas;

@end


@implementation OptionsViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    _optTraversal.selectedSegmentIndex = (int)map->options().traverseMode;
    _optQualityDegrad.value = map->options().maxTexelToPixelScale;
    _optAtmosphere.on = renderOptions.renderAtmosphere;
    _optTouchAreas.on = extraConfig.showControlAreas;
}

- (IBAction)optTraversalChanged:(UISegmentedControl *)sender
{
    map->options().traverseMode = (vts::TraverseMode)sender.selectedSegmentIndex;
}

- (IBAction)optQualityDegradChanged:(UISlider *)sender
{
    map->options().maxTexelToPixelScale = sender.value;
}

- (IBAction)optAtmosphereChanged:(UISwitch *)sender
{
    renderOptions.renderAtmosphere = sender.on;
}

- (IBAction)optTouchAreasChanged:(UISwitch *)sender
{
	extraConfig.showControlAreas = sender.on;
}

@end
