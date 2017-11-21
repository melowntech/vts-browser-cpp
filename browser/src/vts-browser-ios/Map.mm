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
#include <vts-browser/celestial.hpp>
#include <vts-renderer/classes.hpp>

#import <Dispatch/Dispatch.h>
#import <OpenGLES/ES2/gl.h>
#import <dlfcn.h> // dlsym
#import "TimerObj.h"

using namespace vts;
using namespace vts::renderer;

ExtraConfig::ExtraConfig() :
	controlType(0), touchSize(45),
	showControlScales(true), showControlAreas(false)
{
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		controlType = 1;
		touchSize = 90;
		showControlScales = false;
	}
}

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
	std::shared_ptr<Texture> scalesTexturePitch;
	std::shared_ptr<Texture> scalesTextureZoom;
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
        opt.maxResourceProcessesPerTick = -1; // the resources are processed on a separate thread
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
		        scalesTextureYaw->generateMipmaps();
		    }
		    
		    // load texture pitch
		    {
		    	Buffer buff = readInternalMemoryBuffer("data/textures/scale-pitch.png");
		        ResourceInfo info;
		        GpuTextureSpec spec(buff);
		        scalesTexturePitch = std::make_shared<Texture>();
		        scalesTexturePitch->load(info, spec);
		        scalesTexturePitch->generateMipmaps();
		    }
		    
		    // load texture zoom
		    {
		    	Buffer buff = readInternalMemoryBuffer("data/textures/scale-zoom.png");
		        ResourceInfo info;
		        GpuTextureSpec spec(buff);
		        scalesTextureZoom = std::make_shared<Texture>();
		        scalesTextureZoom->load(info, spec);
		        scalesTextureZoom->generateMipmaps();
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
		uvm(0.0) = texturePos.size.width;
		uvm(1,1) = texturePos.size.height;
		uvm(0,2) = texturePos.origin.x;
		uvm(1,2) = texturePos.origin.y;
		scalesShader->uniformMat4(0, mvp.data());
		scalesShader->uniformMat3(1, uvm.data());
		scalesMeshQuad->dispatch();
	}
}

void mapRenderScales(float retinaScale, CGRect whole, CGRect pitch, CGRect yaw, CGRect zoom)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
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
	
	if (extraConfig.showControlScales)
	{
		glEnable(GL_SCISSOR_TEST);
		
		double rotation[3];
		map->getPositionRotation(rotation);
	
		// yaw
		{
		    glScissor(yaw.origin.x * retinaScale, (whole.size.height - yaw.size.height - yaw.origin.y) * retinaScale, yaw.size.width * retinaScale, yaw.size.height * retinaScale);
			scalesTextureYaw->bind();
			float radius = std::min(11.f * retinaScale, (float)yaw.size.height * 0.5f);
			float startX = yaw.origin.x + yaw.size.width * 0.5 - radius;
			float startY = yaw.origin.y + yaw.size.height - 1.7 * radius;
			float singleWidth = 7 * radius;
			float totalWidth = 8 * singleWidth;
			float startOffset = totalWidth * rotation[0] / 360;
			float texH = 0.1;
			// render numbers
			for (int i = -16; i < 10; i++)
			{
				renderQuad(proj,
					CGRectMake(startX + startOffset + i * singleWidth, startY, 2 * radius, 2 * radius),
					CGRectMake(0, ((100 - i + 4) % 8) * texH, 1, texH));
			}
			// render dots
			CGRect dotRect = CGRectMake(0, 8 * texH, 1, texH);
			float dotOffset = startOffset + 0.5 * singleWidth;
			for (int i = -70; i < 30; i++)
			{
				if ((i + 100) % 4 == 2)
					continue;
				renderQuad(proj,
					CGRectMake(startX + dotOffset + (i * singleWidth) / 4, startY, 2 * radius, 2 * radius),
					dotRect);
			}
			// render arrow
			renderQuad(proj,
				CGRectMake(startX, startY, 2 * radius, 1.7 * radius),
				CGRectMake(0, 0.9, 1, texH));
		}
		
		// pitch
		{
		    glScissor(pitch.origin.x * retinaScale, (whole.size.height - pitch.size.height - pitch.origin.y) * retinaScale, pitch.size.width * retinaScale, pitch.size.height * retinaScale);
			scalesTexturePitch->bind();
			float radius = std::min(11.f * retinaScale, (float)pitch.size.width * 0.5f);
			float startX = pitch.origin.x;
			float startY = pitch.origin.y + pitch.size.height * 0.5 - radius;
			float singleHeight = 7 * radius;
			float totalHeight = 8 * singleHeight;
			float startOffset = totalHeight * -rotation[1] / 360;
			float texH = 0.1;
			// render numbers
			for (int i = 6; i < 9; i++)
			{
				renderQuad(proj,
					CGRectMake(startX, startY + startOffset + i * singleHeight, 2 * radius, 2 * radius),
					CGRectMake(0, (i % 8) * texH, 1, texH));
			}
			// render dots
			CGRect dotRect = CGRectMake(0, 8 * texH, 1, texH);
			float dotOffset = startOffset + 0.5 * singleHeight;
			for (int i = 23; i < 30; i++)
			{
				if ((i % 4) == 2)
					continue;
				renderQuad(proj,
					CGRectMake(startX, startY + dotOffset + (i * singleHeight) / 4, 2 * radius, 2 * radius),
					dotRect);
			}
			// render arrow
			renderQuad(proj,
				CGRectMake(startX, startY, 2 * radius, 2 * radius),
				CGRectMake(0, 0.9, 1, texH));
		}
		
		// zoom
		{
		    glScissor(zoom.origin.x * retinaScale, (whole.size.height - zoom.size.height - zoom.origin.y) * retinaScale, zoom.size.width * retinaScale, zoom.size.height * retinaScale);
			scalesTextureZoom->bind();
			float radius = std::min(11.f * retinaScale, (float)zoom.size.width * 0.5f);
			float startX = zoom.origin.x + zoom.size.width - 2 * radius;
			float startY = zoom.origin.y + zoom.size.height * 0.5 - radius;
			float singleHeight = 7 * radius;
			float totalHeight = 8 * singleHeight;
			double zoomVal = map->getPositionViewExtent();
            double zoomScale = map->celestialBody().majorRadius;
			double zoomMin = map->options().viewExtentLimitScaleMin * zoomScale;
			double zoomMax = map->options().viewExtentLimitScaleMax * zoomScale;
			zoomVal = std::log2(zoomVal);
			zoomMin = std::log2(zoomMin);
			zoomMax = std::log2(zoomMax);
			double zoomNorm = (zoomVal - zoomMin) / (zoomMax - zoomMin);
            float startOffset = totalHeight * ((zoomNorm - 1) * 7 / 8);
			float texH = 0.1;
			// render numbers
			for (int i = 0; i < 8; i++)
			{
				renderQuad(proj,
					CGRectMake(startX, startY + startOffset + i * singleHeight, 2 * radius, 2 * radius),
					CGRectMake(0, i * texH, 1, texH));
			}
			// render dots
			CGRect dotRect = CGRectMake(0, 8 * texH, 1, texH);
			float dotOffset = startOffset + 0.5 * singleHeight;
			for (int i = -1; i < 26; i++)
			{
				if ((100 + i) % 4 == 2)
					continue;
				renderQuad(proj,
					CGRectMake(startX, startY + dotOffset + (i * singleHeight) / 4, 2 * radius, 2 * radius),
					dotRect);
			}
			// render arrow
			renderQuad(proj,
				CGRectMake(startX, startY, 2 * radius, 2 * radius),
				CGRectMake(0, 0.9, 1, texH));
		}
		
		glDisable(GL_SCISSOR_TEST);
	}
	
    checkGl("rendered scale");
}

void mapTimerStart(id object, SEL selector)
{
	[timer setObject:object Selector:selector];
}

void mapTimerStop()
{
    [timer setObject:nil Selector:nil];
}


