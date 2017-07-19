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

#include <utility/binaryio.hpp>
#include <utility/uri.hpp>
#include <vts-libs/storage/error.hpp>
#include <vts-libs/registry/io.hpp>
#include <boost/lexical_cast.hpp>
#include "map.hpp"

namespace bin = utility::binaryio;
namespace storage = vtslibs::storage;

namespace vts
{

SriIndex::SriIndex(MapImpl *map, const std::string &name)
 : Resource(map, name, FetchTask::ResourceType::SriIndex)
{
    priority = std::numeric_limits<float>::infinity();
}

void SriIndex::load()
{
    detail::Wrapper is(contentData);

    // load header
    {
        char magic[2];
        bin::read(is, magic);
        if (magic[0] != 'S' || magic[1] != 'R')
        {
            LOGTHROW(err1, storage::BadFileFormat)
                << "File <" << name << "> is not a SRI file.";
        }
        uint16 version;
        bin::read(is, version);
        if (version != 0)
        {
            LOGTHROW(err1, storage::VersionError) << "File <"
                << name << "> has unsupported version (" << version << ").";
        }
    }

    // load metatiles
    {
        uint16 metatilesCount;
        bin::read(is, metatilesCount);
        metatiles.clear();
        metatiles.reserve(metatilesCount);
        for (int i = 0; i < metatilesCount; i++)
        {
            uint16 urlLength;
            bin::read(is, urlLength);
            Buffer urlBuf(urlLength);
            bin::read(is, urlBuf.data(), urlBuf.size());
            std::shared_ptr<MetaTile> m = std::make_shared<MetaTile>(map,
                std::string(urlBuf.data(), urlBuf.size()));
            bin::read(is, m->replyExpires);
            *std::dynamic_pointer_cast<vtslibs::vts::MetaTile>(m)
                    = vtslibs::vts::loadMetaTile(is,
                        map->mapConfig->referenceFrame.metaBinaryOrder,
                        name + "#" + m->name);
            m->state = Resource::State::ready;
            // todo cache the metatile
            metatiles.push_back(m);
        }
    }
}

void SriIndex::update()
{
    LOG(info3) << "Injecting SRI <" << name << ">";
    for (auto &it : metatiles)
    {
        // inject the metatile into the resources
        map->resources.resources[it->name] = it;
    }
    std::vector<std::shared_ptr<MetaTile>>().swap(metatiles);
}

void MapImpl::updateSris()
{
    auto it = resources.sriTasks.begin();
    while (it != resources.sriTasks.end())
    {
        switch (getResourceValidity(*it))
        {
        case Validity::Indeterminate:
            touchResource(*it);
            it++;
            continue;
        case Validity::Invalid:
            break;
        case Validity::Valid:
            (*it)->update();
            break;
        }
        it = resources.sriTasks.erase(it);
    }
}

void MapImpl::initiateSri(const vtslibs::registry::Position *position)
{
    if (resources.sriPath.empty() || options.debugDisableSri)
        return;
    std::stringstream ss;
    ss << resources.sriPath;
    if (resources.sriPath.find("?") == std::string::npos)
        ss << '?';
    else
        ss << '&';
    ss << "mapConfig=" << utility::urlEncode(mapConfigPath);
    if (!resources.authPath.empty())
        ss << "&auth=" << utility::urlEncode(resources.authPath);
    if (position)
        ss << "&pos=" << boost::lexical_cast<std::string>(*position);
    if (!mapConfigView.empty())
        ss << "&view=" << utility::urlEncode(mapConfigView);
    ss << "&vs=" << (options.debugDisableVirtualSurfaces ? "false" : "true");
    ss << "&size=" << renderer.windowWidth << 'x' << renderer.windowHeight;
    std::string name = ss.str();
    LOG(info3) << "Initiating SRI <" << name << ">";
    resources.sriTasks.push_back(getSriIndex(name));
}

} // namespace vts
