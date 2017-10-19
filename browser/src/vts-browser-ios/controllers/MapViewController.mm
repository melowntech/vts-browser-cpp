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

#include "../Map.h"
#include <vts-browser/draws.hpp>
#include <vts-renderer/renderer.hpp>

#import "MapViewController.h"

@interface MapViewController ()
{
	bool fullscreenStatus;
	bool fullscreenOverride;
}

@property (weak, nonatomic) IBOutlet UIView *gestureViewCenter;
@property (weak, nonatomic) IBOutlet UIView *gestureViewBottom;
@property (weak, nonatomic) IBOutlet UIView *gestureViewLeft;
@property (weak, nonatomic) IBOutlet UIView *gestureViewRight;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *activityIndicator;

@property (strong, nonatomic) UIBarButtonItem *searchButton;

@end


@implementation MapViewController

// gestures

- (void)gesturePan:(UIPanGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
		case UIGestureRecognizerStateEnded:
		case UIGestureRecognizerStateChanged:
		{
			CGPoint p = [recognizer translationInView:_gestureViewCenter];
			[recognizer setTranslation:CGPoint() inView:_gestureViewCenter];
			map->pan({p.x * 2, p.y * 2, 0});
			fullscreenOverride = false;
		} break;
		default:
			break;
	}
}

- (void)gestureYaw:(UIPanGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
		case UIGestureRecognizerStateEnded:
		case UIGestureRecognizerStateChanged:
		{
			CGPoint p = [recognizer translationInView:_gestureViewCenter];
			[recognizer setTranslation:CGPoint() inView:_gestureViewCenter];
			map->rotate({2000 * p.x / renderOptions.width, 0, 0});
			fullscreenOverride = false;
		} break;
		default:
			break;
	}
}

- (void)gesturePitch:(UIPanGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
		case UIGestureRecognizerStateEnded:
		case UIGestureRecognizerStateChanged:
		{
			CGPoint p = [recognizer translationInView:_gestureViewCenter];
			[recognizer setTranslation:CGPoint() inView:_gestureViewCenter];
			map->rotate({0, 2000 * p.y / renderOptions.height, 0});
			fullscreenOverride = false;
		} break;
		default:
			break;
	}
}

- (void)gestureZoom:(UIPanGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
		case UIGestureRecognizerStateEnded:
		case UIGestureRecognizerStateChanged:
		{
			CGPoint p = [recognizer translationInView:_gestureViewCenter];
			[recognizer setTranslation:CGPoint() inView:_gestureViewCenter];
			map->zoom(-100 * p.y / renderOptions.height);
			fullscreenOverride = false;
		} break;
		default:
			break;
	}
}

- (void)gestureNorthUp:(UILongPressGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
        case UIGestureRecognizerStateBegan:
            map->setPositionRotation({0,270,0}, vts::NavigationType::Quick);
            map->resetNavigationMode();
			fullscreenOverride = false;
			break;
		default:
			break;
	}
}

- (void)gestureFullscreen:(UITapGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
        case UIGestureRecognizerStateEnded:
			fullscreenOverride = !fullscreenOverride;
			break;
		default:
			break;
	}
}

// controller status

- (BOOL)prefersStatusBarHidden
{
	if (fullscreenStatus)
		return true;
	return [super prefersStatusBarHidden];
}

- (void)updateFullscreen
{
	bool enable = !fullscreenOverride;
	fullscreenStatus = enable;
	[self setNeedsStatusBarAppearanceUpdate];
    [self.navigationController setNavigationBarHidden:enable animated:YES];
}

- (void)configureView
{
    self.title = _item.name;
    {
        std::string url([_item.url UTF8String]);
        map->setMapConfigPath(url);
    }
}

- (void)setItem:(ConfigItem*)item
{
    _item = item;
}

- (void)showSearch
{
	[self performSegueWithIdentifier:@"search" sender:self];
}

- (void)showOptions
{
	[self performSegueWithIdentifier:@"position" sender:self];
}

// view controls

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    _gestureViewBottom.constraints.firstObject.constant = extraConfig.touchSize;
    _gestureViewLeft.constraints.firstObject.constant = extraConfig.touchSize;
    _gestureViewRight.constraints.firstObject.constant = extraConfig.touchSize;
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
	[self updateFullscreen];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _searchButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSearch target:self action:@selector(showSearch)];
    self.navigationItem.rightBarButtonItems = @[
    	[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemOrganize target:self action:@selector(showOptions)],
    	_searchButton
    ];
    
    fullscreenStatus = false;
    fullscreenOverride = true;
    
    // initialize rendering
    GLKView *view = (GLKView *)self.view;
    view.context = mapRenderContext();
    
    // initialize gesture recognizers
    {
	    // pan recognizer
	    assert(_gestureViewCenter);
		UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gesturePan:)];
        [_gestureViewCenter addGestureRecognizer:r];
    }
    {
	    // yaw recognizer
	    assert(_gestureViewBottom);
		UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gestureYaw:)];
        [_gestureViewBottom addGestureRecognizer:r];
    }
    {
	    // pitch recognizer
	    assert(_gestureViewLeft);
		UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gesturePitch:)];
        [_gestureViewLeft addGestureRecognizer:r];
    }
    {
	    // zoom recognizer
	    assert(_gestureViewRight);
		UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gestureZoom:)];
        [_gestureViewRight addGestureRecognizer:r];
    }
    {
    	// north up recognizer
		UILongPressGestureRecognizer *r = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(gestureNorthUp:)];
        [view addGestureRecognizer:r];
    }
    {
    	// fullscreen
		UITapGestureRecognizer *r = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(gestureFullscreen:)];
        [view addGestureRecognizer:r];
    }
    
    [self configureView];
}

// rendering

- (void)update
{
	mapTimerStop();

	[self updateFullscreen];
	
	if (map->getMapConfigReady())
	    [_activityIndicator stopAnimating];
	else
        [_activityIndicator startAnimating];
	
	_searchButton.enabled = map->searchable();

    map->renderTickPrepare();
    map->renderTickRender();
}

- (void)glkView:(nonnull GLKView *)view drawInRect:(CGRect)rect
{
    {
        GLint fbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
        renderOptions.targetFrameBuffer = fbo;
    }
    
	vts::renderer::render(renderOptions, map->draws(), map->celestialBody());
	mapRenderScales(view.contentScaleFactor, rect, _gestureViewLeft.frame, _gestureViewBottom.frame, _gestureViewRight.frame);
    
    renderOptions.targetViewportX = rect.origin.x * view.contentScaleFactor;
    renderOptions.targetViewportY = rect.origin.y * view.contentScaleFactor;
    renderOptions.width = rect.size.width * view.contentScaleFactor;
    renderOptions.height = rect.size.height * view.contentScaleFactor;
    map->setWindowSize(renderOptions.width, renderOptions.height);
}

@end

