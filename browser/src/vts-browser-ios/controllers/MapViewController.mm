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
#include <vts-browser/math.hpp>
#include <vts-renderer/renderer.hpp>

#import "MapViewController.h"

using namespace vts;


@interface MapViewController ()
{
	vec3 gotoPoint;
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

- (void)gestureYawPan:(UIPanGestureRecognizer*)recognizer
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

- (void)gestureYawRot:(UIRotationGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
		case UIGestureRecognizerStateEnded:
		case UIGestureRecognizerStateChanged:
		{
			map->rotate({-400 * recognizer.rotation, 0, 0});
			[recognizer setRotation:0];
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

- (void)gestureZoomPan:(UIPanGestureRecognizer*)recognizer
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

- (void)gestureZoomPinch:(UIPinchGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
		case UIGestureRecognizerStateEnded:
		case UIGestureRecognizerStateChanged:
		{
			map->zoom(10 * (recognizer.scale - 1));
			[recognizer setScale:1];
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

- (void)gestureGoto:(UITapGestureRecognizer*)recognizer
{
    switch (recognizer.state)
	{
        case UIGestureRecognizerStateEnded:
	    {
	    	CGPoint point = [recognizer locationInView:self.view];
			float x = point.x * self.view.contentScaleFactor;
			float y = point.y * self.view.contentScaleFactor;
	    	gotoPoint = vec3(x, y, 0);
			fullscreenOverride = false;
	    } break;
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

- (void)removeAllGestures:(UIView*)view
{
	if (!view.gestureRecognizers)
		return;
    while (view.gestureRecognizers.count)
        [view removeGestureRecognizer:[view.gestureRecognizers lastObject]];
}

- (void)initializeGestures
{
	assert(_gestureViewCenter);
	assert(_gestureViewBottom);
	assert(_gestureViewLeft);
	assert(_gestureViewRight);
	
	[self removeAllGestures:self.view];
	[self removeAllGestures:_gestureViewCenter];
	[self removeAllGestures:_gestureViewBottom];
	[self removeAllGestures:_gestureViewLeft];
	[self removeAllGestures:_gestureViewRight];
	
	if (extraConfig.controlType == 0)
	{
		// single touch
		{
			// pan recognizer
			UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gesturePan:)];
		    [_gestureViewCenter addGestureRecognizer:r];
		}
		{
			// yaw recognizer
			UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gestureYawPan:)];
		    [_gestureViewBottom addGestureRecognizer:r];
		}
		{
			// pitch recognizer
			UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gesturePitch:)];
		    [_gestureViewLeft addGestureRecognizer:r];
		}
		{
			// zoom recognizer
			UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gestureZoomPan:)];
		    [_gestureViewRight addGestureRecognizer:r];
		}
    }
    else
    {
    	// multitouch
		{
			// pan recognizer
			UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gesturePan:)];
			r.maximumNumberOfTouches = 1;
		    [self.view addGestureRecognizer:r];
		}
		{
			// yaw recognizer
			UIRotationGestureRecognizer *r = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(gestureYawRot:)];
		    [self.view addGestureRecognizer:r];
		}
		{
			// pitch recognizer
			UIPanGestureRecognizer *r = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(gesturePitch:)];
			r.minimumNumberOfTouches = 2;
		    [self.view addGestureRecognizer:r];
		}
		{
			// zoom recognizer
			UIPinchGestureRecognizer *r = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(gestureZoomPinch:)];
		    [self.view addGestureRecognizer:r];
		}
    }
    {
    	// north up recognizer
		UILongPressGestureRecognizer *r = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(gestureNorthUp:)];
        [self.view addGestureRecognizer:r];
    }
    {
    	// goto
		UITapGestureRecognizer *r = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(gestureGoto:)];
		r.numberOfTapsRequired = 2;
        [self.view addGestureRecognizer:r];
    }
    {
    	// fullscreen
		UITapGestureRecognizer *r = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(gestureFullscreen:)];
        [self.view addGestureRecognizer:r];
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
    [self initializeGestures];
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
    
    gotoPoint(0) = std::numeric_limits<float>::quiet_NaN();
    fullscreenStatus = false;
    fullscreenOverride = true;
    
    // initialize rendering
    GLKView *view = (GLKView *)self.view;
    view.context = mapRenderContext();
    
    [self configureView];
}

// rendering

- (vec3)worldPositionFromScreen:(vec3)screenPos
{
	float x = screenPos(0);
	float y = screenPos(1);
    y = renderOptions.height - y - 1;
    float depth = std::numeric_limits<float>::quiet_NaN();
    glReadPixels((int)x, (int)y, 1, 1,
                 GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    renderer::checkGl("glReadPixels");
    if (depth > 1 - 1e-7)
        depth = std::numeric_limits<float>::quiet_NaN();
    depth = depth * 2 - 1;
    x = x / renderOptions.width * 2 - 1;
    y = y / renderOptions.height * 2 - 1;
    mat4 viewProj = rawToMat4(map->draws().camera.proj)
            * rawToMat4(map->draws().camera.view);
    return vec4to3(viewProj.inverse() * vec4(x, y, depth, 1), true);
}

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
	
	if (gotoPoint(0) == gotoPoint(0))
	{
		// todo fix error in glReadPixels
		/*
        vec3 posPhys = [self worldPositionFromScreen:gotoPoint];
        if (posPhys(0) == posPhys(0))
        {
            double posNav[3];
            map->convert(posPhys.data(), posNav,
                         Srs::Physical, Srs::Navigation);
            map->setPositionPoint(posNav, NavigationType::Quick);
        }
        */
        gotoPoint(0) = std::numeric_limits<float>::quiet_NaN();
    }
	
	mapRenderScales(view.contentScaleFactor, rect, _gestureViewLeft.frame, _gestureViewBottom.frame, _gestureViewRight.frame);
    
    renderOptions.targetViewportX = rect.origin.x * view.contentScaleFactor;
    renderOptions.targetViewportY = rect.origin.y * view.contentScaleFactor;
    renderOptions.width = rect.size.width * view.contentScaleFactor;
    renderOptions.height = rect.size.height * view.contentScaleFactor;
    map->setWindowSize(renderOptions.width, renderOptions.height);
}

@end

