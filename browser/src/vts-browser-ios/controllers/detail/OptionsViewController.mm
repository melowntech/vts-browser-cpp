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
@property (weak, nonatomic) IBOutlet UISegmentedControl *optControlType;
@property (weak, nonatomic) IBOutlet UISlider *optTouchSize;
@property (weak, nonatomic) IBOutlet UISwitch *optTouchAreas;
@property (weak, nonatomic) IBOutlet UISwitch *optControlScales;
@property (weak, nonatomic) IBOutlet UISwitch *optControlCompas;

@end


@implementation OptionsViewController

- (void)updateShowControls
{
    extraConfig.showControlScales = extraConfig.controlType == 0;
    extraConfig.showControlCompas = extraConfig.controlType == 1;
}

- (void)updateViewControls
{
    _optTouchSize.enabled = extraConfig.controlType == 0;
    _optTouchAreas.enabled = extraConfig.controlType == 0;
    if (!_optTouchAreas.enabled)
        extraConfig.showControlAreas = false;
    //if (extraConfig.showControlScales && extraConfig.showControlCompas)
    //    [self updateShowControls];
    _optTouchSize.value = extraConfig.touchSize;
    _optTouchAreas.on = extraConfig.showControlAreas;
    _optControlScales.on = extraConfig.showControlScales;
    _optControlCompas.on = extraConfig.showControlCompas;
    _optControlType.selectedSegmentIndex = extraConfig.controlType;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // rendering
    _optTraversal.selectedSegmentIndex = (int)map->options().traverseMode;
    _optQualityDegrad.value = map->options().maxTexelToPixelScale;
    _optAtmosphere.on = renderOptions.renderAtmosphere;
    // controls
    [self updateViewControls];
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

- (IBAction)optControlTypeChanged:(UISegmentedControl *)sender
{
    extraConfig.controlType = sender.selectedSegmentIndex;
    [self updateShowControls];
    [self updateViewControls];
}

- (IBAction)optTouchSizeChanged:(UISlider *)sender
{
    extraConfig.touchSize = sender.value;
}

- (IBAction)optTouchAreasChanged:(UISwitch *)sender
{
    extraConfig.showControlAreas = sender.on;
}

- (IBAction)optControlScalesChanged:(UISwitch *)sender
{
    extraConfig.showControlScales = sender.on;
    [self updateViewControls];
}

- (IBAction)optControlCompasChanged:(UISwitch *)sender
{
    extraConfig.showControlCompas = sender.on;
    [self updateViewControls];
}

@end

