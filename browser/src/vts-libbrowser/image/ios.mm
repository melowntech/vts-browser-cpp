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

#include <dbglog/dbglog.hpp>
#include "../include/vts-browser/buffer.hpp"

#import <UIKit/UIKit.h>

namespace vts
{

namespace
{

void decodeImg(const Buffer &in, Buffer &out,
               uint32 &width, uint32 &height, uint32 &components, int type)
{
	CGDataProviderRef provider = nullptr;
	CGImageRef image = nullptr;
	try
	{
		provider = CGDataProviderCreateWithData(NULL, in.data(), in.size(), NULL);
		switch(type)
		{
		case 0:
			image = CGImageCreateWithPNGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
			break;
		case 1:
			image = CGImageCreateWithJPEGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
			break;
		default:
			LOGTHROW(fatal, std::logic_error) << "invalid image type";
		}
		if (CGImageGetBitsPerComponent(image) != 8)
			LOGTHROW(err1, std::runtime_error) << "invalid image bits-per-component <" << CGImageGetBitsPerComponent(image) << ">";
		width = CGImageGetWidth(image);
		height = CGImageGetHeight(image);
		components = CGImageGetBitsPerPixel(image) / 8;
		if (components < 1 || components > 4)
			LOGTHROW(err1, std::runtime_error) << "invalid image components count <" << components << ">";
		NSData *data = (id)CGDataProviderCopyData(CGImageGetDataProvider(image));
		try
		{
			size_t size = [data length];
			assert(size == components * width * height);
			out.allocate(size);
			memcpy(out.data(), [data bytes], out.size());
		}
		catch(...)
		{
			[data release];
			throw;
		}
		[data release];
	}
	catch(...)
	{
		CGImageRelease(image);
		CGDataProviderRelease(provider);
		throw;
	}
	CGImageRelease(image);
    CGDataProviderRelease(provider);
}

} // namespace



void decodePng(const Buffer &in, Buffer &out,
               uint32 &width, uint32 &height, uint32 &components)
{
	decodeImg(in, out, width, height, components, 0);
}

void encodePng(const Buffer &in, Buffer &out,
               uint32 width, uint32 height, uint32 components)
{
    LOGTHROW(fatal, std::logic_error) << "encode png not implemented";
}

void decodeJpeg(const Buffer &in, Buffer &out,
                uint32 &width, uint32 &height, uint32 &components)
{
	decodeImg(in, out, width, height, components, 1);
}

} // namespace vts

