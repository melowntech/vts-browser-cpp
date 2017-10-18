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

#include <cassert>
#include <unistd.h> // usleep
#include "Map.h"
#include <vts-browser/math.hpp>
#include <vts-browser/options.hpp>
#include <vts-browser/fetcher.hpp>
#include <vts-renderer/classes.hpp>

#import <Dispatch/Dispatch.h>
#import <OpenGLES/ES2/gl.h>
#import <dlfcn.h> // dlsym


@interface TimerObj : NSObject

- (void)setObject:(id)object Selector:(SEL)selector;

@end

@interface TimerObj ()
{
	NSTimer* timer;
    id object;
    SEL selector;
}
@end

@implementation TimerObj

- (id)init
{
    if (self = [super init])
    {
		timer = [NSTimer timerWithTimeInterval:0.2 target:self selector:@selector(timerTick) userInfo:nil repeats:YES];
		[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
	}
	return self;
}

- (void)timerTick
{
	if (!object || !map)
		return;
    map->renderTickPrepare();
    map->renderTickRender();
	[object performSelector:selector];
}

- (void)setObject:(id)object Selector:(SEL)selector
{
	self->object = object;
	self->selector = selector;
}

@end

using namespace vts;
using namespace vts::renderer;

ExtraConfig::ExtraConfig() : showControlAreas(false)
{}

Map *map;
RenderOptions renderOptions;
ExtraConfig extraConfig; 

namespace
{
	dispatch_queue_t dataQueue;
	EAGLContext *dataContext;
	EAGLContext *renderContext;
	std::shared_ptr<Shader> scalesShader;
	std::shared_ptr<Mesh> scalesMeshQuad;
	std::shared_ptr<Texture> textureControlAreas;
	std::shared_ptr<Texture> scalesTextureYaw;
	TimerObj *timer;
	
	void dataUpdate()
	{
		// queue next iteration
		dispatch_async(dataQueue,
		^{
			dataUpdate();
		});
		
		// process some resources (which may require opengl context)
		[EAGLContext setCurrentContext:dataContext];
		map->dataTick();
		[EAGLContext setCurrentContext:nullptr];
		
		// save some cpu cycles
        usleep(150000);
	}
	
	void *iosGlGetProcAddress(const char *name)
	{
		return dlsym(RTLD_DEFAULT, name);
	}
}

void mapInitialize()
{
	// create the map
	{
        MapCreateOptions createOptions;
        createOptions.clientId = "vts-browser-ios";
        createOptions.disableCache = true;
        assert(!map);
        map = new Map(createOptions);
    }
    
    // configure the map for mobile use
    {
        MapOptions &opt = map->options();
        opt.maxTexelToPixelScale = 3.2;
        opt.maxBalancedCoarsenessScale = 5.3;
        opt.maxResourceProcessesPerTick = -1; // the resources are processed on a separate thread
        //opt.traverseMode = vts::TraverseMode::Hierarchical;
    }
    
    // configure opengl contexts
    {
		dataContext = [[EAGLContext alloc] initWithAPI: kEAGLRenderingAPIOpenGLES3];
		renderContext = [[EAGLContext alloc] initWithAPI: kEAGLRenderingAPIOpenGLES3 sharegroup:dataContext.sharegroup];
		[EAGLContext setCurrentContext:renderContext];

		loadGlFunctions(&iosGlGetProcAddress);
		vts::renderer::initialize();
		map->renderInitialize();
		
		map->callbacks().loadTexture = std::bind(&loadTexture, std::placeholders::_1, std::placeholders::_2);
		map->callbacks().loadMesh = std::bind(&loadMesh, std::placeholders::_1, std::placeholders::_2);
		
		// load data for rendering scales
		{
		    // load shader
		    {
		        scalesShader = std::make_shared<Shader>();
		        Buffer vert = readInternalMemoryBuffer("data/shaders/scales.vert.glsl");
		        Buffer frag = readInternalMemoryBuffer("data/shaders/scales.frag.glsl");
		        scalesShader->load(
		            std::string(vert.data(), vert.size()),
		            std::string(frag.data(), frag.size()));
		        std::vector<uint32> &uls = scalesShader->uniformLocations;
		        GLuint id = scalesShader->getId();
		        uls.push_back(glGetUniformLocation(id, "uniMvp"));
		        uls.push_back(glGetUniformLocation(id, "uniUvm"));
		        glUseProgram(id);
		        glUniform1i(glGetUniformLocation(id, "uniTexture"), 0);
		        checkGl();
		    }

		    // load mesh
		    {
		    	Buffer buff = readInternalMemoryBuffer("data/meshes/rect.obj");
		        ResourceInfo info;
		        GpuMeshSpec spec(buff);
		        spec.attributes.resize(2);
				spec.attributes[0].enable = true;
				spec.attributes[0].stride = sizeof(vec3f) + sizeof(vec2f);
				spec.attributes[0].components = 3;
				spec.attributes[1].enable = true;
				spec.attributes[1].stride = spec.attributes[0].stride;
				spec.attributes[1].components = 2;
				spec.attributes[1].offset = sizeof(vec3f);
		        scalesMeshQuad = std::make_shared<Mesh>();
		        scalesMeshQuad->load(info, spec);
		    }
		    
		    // load texture control areas
		    {
		    	Buffer buff = readInternalMemoryBuffer("data/textures/border.png");
		        ResourceInfo info;
		        GpuTextureSpec spec(buff);
		        textureControlAreas = std::make_shared<Texture>();
		        textureControlAreas->load(info, spec);
		    }
		    
		    // load texture yaw
		    {
		    	Buffer buff = readInternalMemoryBuffer("data/textures/scale-yaw.png");
		        ResourceInfo info;
		        GpuTextureSpec spec(buff);
		        scalesTextureYaw = std::make_shared<Texture>();
		        scalesTextureYaw->load(info, spec);
		    }
		}
		
		[EAGLContext setCurrentContext:nullptr];
    }
    
    // create data thread
	dataQueue = dispatch_queue_create("com.melown.vts.map.data", NULL);
	dispatch_async(dataQueue,
	^{	
	    map->dataInitialize(vts::Fetcher::create(vts::FetcherOptions()));
	    dataUpdate();
	});
	
	// prepare timer
	timer = [[TimerObj alloc] init];
}

EAGLContext *mapRenderContext()
{
	return renderContext;
}

namespace
{
	void renderQuad(mat4 proj, CGRect screenPos, CGRect texturePos)
	{
		mat4 model = identityMatrix4();
		model(0,0) = screenPos.size.width;
		model(1,1) = screenPos.size.height;
		model(0,3) = screenPos.origin.x;
		model(1,3) = screenPos.origin.y;
		mat4f mvp = (proj * model).cast<float>();
		mat3f uvm = identityMatrix3().cast<float>();
		scalesShader->uniformMat4(0, mvp.data());
		scalesShader->uniformMat3(1, uvm.data());
		scalesMeshQuad->dispatch();
	}
}

void mapRenderScales(float retinaScale, CGRect whole, CGRect pitch, CGRect yaw, CGRect zoom)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(whole.origin.x * retinaScale, whole.origin.y * retinaScale, whole.size.width * retinaScale, whole.size.height * retinaScale);
    glActiveTexture(GL_TEXTURE0);
    
	scalesTextureYaw->bind();
	scalesShader->bind();
	scalesMeshQuad->bind();
	mat4 proj = orthographicMatrix(0, whole.size.width, whole.size.height, 0, 0, 1);
    checkGl("prepare render scale");
	
	if (extraConfig.showControlAreas)
	{
		textureControlAreas->bind();
		renderQuad(proj, pitch, CGRectMake(0, 0, 1, 1));
		renderQuad(proj, yaw, CGRectMake(0, 0, 1, 1));
		renderQuad(proj, zoom, CGRectMake(0, 0, 1, 1));
	}
	
	// yaw
	{
		scalesTextureYaw->bind();
	}
	
    checkGl("render scale");
}

void mapTimerStart(id object, SEL selector)
{
	[timer setObject:object Selector:selector];
}

void mapTimerStop()
{
    [timer setObject:nil Selector:nil];
}


