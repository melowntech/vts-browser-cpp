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

#include "../include/vts-browser/mapOptions.hpp"
#include "../resources.hpp"
#include "../map.hpp"

#include <boost/filesystem.hpp>
#include <utility/path.hpp> // homeDir
#include <utility/md5.hpp>
#include <dbglog/dbglog.hpp>
#include <optick.h>

namespace vts
{

namespace
{

static const char Magic[] = "vtscache";
static const uint16 Version = 4;

enum class CacheFlags : uint16
{
    None = 0,
    AvailFailed = 1 << 0,
};

struct CacheHeader
{
    char magic[16];
    uint16 version;
    uint16 flags;
    uint16 nameLen;
    sint64 expires;
};

char digit(unsigned char a)
{
    assert(a < 16);
    if (a >= 10)
        return a + 'A' - 10;
    return a + '0';
}

} // namespace

class Cache
{
public:
    Cache(const MapCreateOptions &options) :
        root(options.cachePath),
        disabled(!options.diskCache),
        hashes(options.hashCachePaths)
    {
        if (options.diskCache)
        {
#ifdef __EMSCRIPTEN__
            LOGTHROW(err4, std::logic_error)
                << "Disk Cache is not awailable in WASM";
#else
            if (root.empty())
            {
                root = "/home";
                root = utility::homeDir().string();
                if (root.empty())
                {
                    LOGTHROW(err3, std::runtime_error)
                        << "Invalid home dir, the cache path must be defined";
                }
                root += "/.cache/vts-browser/";
            }
            if (root.back() != '/')
                root += "/";
            LOG(info2) << "Disk cache path: <" << root << ">";
#endif
        }
    }

    void write(const CacheData &cd)
    {
#ifndef __EMSCRIPTEN__
        if (disabled)
            return;
        OPTICK_EVENT();
        try
        {
            std::string name = stripScheme(cd.name);
            Buffer b(sizeof(CacheHeader) + name.size() + cd.buffer.size());
            memset(b.data(), 0, sizeof(CacheHeader)); // initialize structure padding
            CacheHeader *h = (CacheHeader*)b.data();
            memcpy(h->magic, Magic, sizeof(Magic));
            h->version = Version;
            if (cd.availFailed)
                h->flags |= (uint16)CacheFlags::AvailFailed;
            h->expires = cd.expires;
            h->nameLen = name.size();
            memcpy(b.data() + sizeof(CacheHeader), name.data(), name.size());
            memcpy(b.data() + sizeof(CacheHeader) + name.size(),
                cd.buffer.data(), cd.buffer.size());
            writeLocalFileBuffer(convertNameToCache(name), b);
        }
        catch (...)
        {
            // do nothing
        }
#endif
    }

    CacheData read(const std::string &nameParam)
    {
#ifdef __EMSCRIPTEN__
        return {};
#else
        if (disabled)
            return {};
        OPTICK_EVENT();
        std::string name = stripScheme(nameParam);
        std::string fileName = convertNameToCache(name);
        if (!boost::filesystem::exists(fileName))
            return {};
        try
        {
            CacheData cd;
            Buffer b = readLocalFileBuffer(fileName);
            if (b.size() < sizeof(CacheHeader))
                return {};
            CacheHeader *h = (CacheHeader*)b.data();
            if (memcmp(h->magic, Magic, sizeof(Magic)) != 0)
                return {};
            if (h->version != Version)
                return {};
            sint64 &expires = cd.expires;
            expires = h->expires;
            if (expires == -2)
                return {}; // must revalidate
            if (expires > 0 && expires < std::time(nullptr))
                return {}; // expired
            if (name.size() != h->nameLen)
                return {};
            if (b.size() < sizeof(CacheHeader) + h->nameLen)
                return {};
            if (memcmp(b.data() + sizeof(CacheHeader),
                name.data(), h->nameLen) != 0)
                return {};
            uint32 size = b.size() - sizeof(CacheHeader) - h->nameLen;
            if (size > 0)
            {
                cd.buffer.allocate(size);
                memcpy(cd.buffer.data(), b.data()
                    + sizeof(CacheHeader) + h->nameLen, size);
            }
            cd.availFailed = (h->flags & (uint16)CacheFlags::AvailFailed)
                == (uint16)CacheFlags::AvailFailed;
            cd.name = nameParam;
            return cd;
        }
        catch (...)
        {
            return {};
        }
#endif
    }

    void purge()
    {
#ifndef __EMSCRIPTEN__
        if (disabled)
            return;
        OPTICK_EVENT();
        LOG(info2) << "Purging disk cache";
        assert(root.length() > 0 && root[root.length() - 1] == '/');
        std::string op = root.substr(0, root.length() - 1);
        if (!boost::filesystem::exists(op))
            return;
        try
        {
            std::string np = op + "-deleted";
            boost::filesystem::rename(op, np);
            boost::filesystem::remove_all(np);
        }
        catch (const std::exception &e)
        {
            LOG(warn3) << "Purging cache failed: <" << e.what() << ">";
        }
#endif
    }

    std::string convertNameToCache(const std::string &path)
    {
        assert(path == stripScheme(path));
        if (hashes)
        {
            unsigned char digest[16];
            utility::md5::hash(path.data(), path.size(), (char*)digest);
            std::string r = root;
            for (int i = 0; i < 16; i++)
            {
                r += digit(digest[i] / 16);
                r += digit(digest[i] % 16);
                if (i == 0 || i == 1)
                    r += '/';
            }
            return r;
        }
        else
        {
            std::string b, c;
            std::string d = convertNameToFolderAndFile(path, b, c);
            if (b.empty() || c.empty())
                LOGTHROW(err2, std::runtime_error)
                        << "Cannot convert path '" << path
                        << "' into a cache path";
            return root + d;
        }
    }

    std::string stripScheme(const std::string &name)
    {
        auto p = name.find("://");
        return p == std::string::npos ? name : name.substr(p + 3);
    }

    std::string root;
    const bool disabled;
    const bool hashes;
};

void Resources::cacheInit()
{
    map->cache = std::make_shared<Cache>(map->createOptions);
}

void Resources::cacheWrite(const CacheData &data)
{
    map->cache->write(data);
}

CacheData Resources::cacheRead(const std::string &name)
{
    return map->cache->read(name);
}

void Resources::purgeResourcesCache()
{
    map->cache->purge();
}

std::string convertNameToPath(const std::string &pathParam, bool preserveSlashes)
{
    std::string path = boost::filesystem::path(pathParam)
        .normalize().string();
    std::string res;
    res.reserve(path.size());
    for (char it : path)
    {
        if ((it >= 'a' && it <= 'z')
         || (it >= 'A' && it <= 'Z')
         || (it >= '0' && it <= '9')
         || (it == '-' || it == '.'))
            res += it;
        else if (preserveSlashes && (it == '/' || it == '\\'))
            res += '/';
        else
            res += '_';
    }
    return res;
}

std::string convertNameToFolderAndFile(const std::string &path,
                    std::string &folder, std::string &file)
{
    std::string b = boost::filesystem::path(path).parent_path().string();
    std::string c = path.substr(b.length() + 1);
    folder = convertNameToPath(b, false);
    file = convertNameToPath(c, false);
    return folder + "/" + file;
}

} // namespace vts
