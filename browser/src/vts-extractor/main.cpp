/**
 * Copyright (c) 2020 Melown Technologies SE
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

#include <cstdio>
#include <cstdlib>
#include <vts-browser/log.hpp>
#include <vts-browser/map.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/mapCallbacks.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/cameraOptions.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/position.hpp>

int main(int argc, char *argv[])
{
    vts::setLogThreadName("main");
    vts::setLogFile("vts-extractor.log");
    //vts::setLogMask(vts::LogLevel::all);

    if (argc != 3)
        return 1;

    vts::log(vts::LogLevel::info4, "initializing");
    vts::MapCreateOptions createOptions;
    createOptions.clientId = "vts-extractor";
    vts::Map map(createOptions);
    map.callbacks().loadMesh = [](vts::ResourceInfo &,
        vts::GpuMeshSpec &, const std::string &id) -> void {};
    map.callbacks().loadTexture = [](vts::ResourceInfo &,
        vts::GpuTextureSpec &, const std::string &id) -> void {};
    map.options().debugExtractRawResources = true;
    map.setMapconfigPath(argv[1]);

    auto camera = map.createCamera();
    camera->options().traverseModeSurfaces = vts::TraverseMode::Fixed;
    camera->options().traverseModeGeodata = vts::TraverseMode::None;
    camera->options().fixedTraversalDistance = 100;
    camera->options().fixedTraversalLod = 22;
    camera->setViewportSize(100, 100);

    auto navigation = camera->createNavigation();
    map.callbacks().mapconfigAvailable = [&]() {
        vts::log(vts::LogLevel::info4, "setting the position");
        navigation->setPosition(vts::Position(argv[2]));
    };

    vts::log(vts::LogLevel::info4, "starting the loop");
    int completed = 0;
    while (true)
    {
        map.dataUpdate();
        map.renderUpdate(1);
        camera->renderUpdate();
        if (map.getMapRenderComplete())
        {
            if (++completed == 10)
                break;
        }
        else
            completed = 0;
    }

    vts::log(vts::LogLevel::info4, "finalizing");
    map.renderFinalize();
    return 0;
}

