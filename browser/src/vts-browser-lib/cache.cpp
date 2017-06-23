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

#include <boost/filesystem.hpp>
#include <utility/path.hpp> // homeDir
#include <utility/md5.hpp>
#include <dbglog/dbglog.hpp>
#include "include/vts-browser/options.hpp"

#include "cache.hpp"

namespace vts
{

namespace
{

static const char Magic[] = "vtscache";
static const uint16 Version = 2;

struct CacheHeader
{
    char magic[sizeof(Magic)];
    uint16 version;
    std::time_t expires;
    uint16 nameLen;
};

char digit(unsigned char a)
{
    assert(a < 16);
    if (a >= 10)
        return a + 'A' - 10;
    return a + '0';
}

class CacheImpl : public Cache
{
public:
    CacheImpl(const MapCreateOptions &options) :
        root(options.cachePath), disabled(options.disableCache),
        hashes(options.hashCachePaths)
    {
        if (!disabled)
        {
            if (root.empty())
            {
                root = utility::homeDir().string();
                if (root.empty())
                {
                    LOGTHROW(err3, std::runtime_error)
                        << "invalid home dir, the cache path must be defined";
                }
                root += "/.cache/vts-browser/";
            }
            if (root.back() != '/')
                root += "/";
        }
    }

    void write(std::string name,
               const Buffer &buffer, time_t expires) override
    {
        if (disabled)
            return;
        name = stripScheme(name);
        try
        {
            Buffer b(sizeof(CacheHeader) + name.size() + buffer.size());
            memset(b.data(), 0, sizeof(CacheHeader)); // initialize structure padding
            CacheHeader *h = (CacheHeader*)b.data();
            memcpy(h->magic, Magic, sizeof(Magic));
            h->version = Version;
            h->expires = expires;
            h->nameLen = name.size();
            memcpy(b.data() + sizeof(CacheHeader), name.data(), name.size());
            memcpy(b.data() + sizeof(CacheHeader) + name.size(),
                   buffer.data(), buffer.size());
            writeLocalFileBuffer(convertNameToCache(name), b);
        }
        catch (...)
        {
            // do nothing
        }
    }

    bool read(std::string name,
              Buffer &buffer, time_t &expires) override
    {
        assert(buffer.size() == 0);
        if (disabled)
            return false;
        name = stripScheme(name);
        std::string cn = convertNameToCache(name);
        if (!boost::filesystem::exists(cn))
            return false;
        try
        {
            Buffer b = readLocalFileBuffer(cn);
            if (b.size() < sizeof(CacheHeader))
                return false;
            CacheHeader *h = (CacheHeader*)b.data();
            if (memcmp(h->magic, Magic, sizeof(Magic)) != 0)
                return false;
            if (h->version != Version)
                return false;
            expires = h->expires;
            if (expires == -2)
                return false; // must revalidate
            if (expires > 0 && expires < std::time(nullptr))
                return false;
            if (name.size() != h->nameLen)
                return false;
            if (b.size() < sizeof(CacheHeader) + h->nameLen)
                return false;
            if (memcmp(b.data() + sizeof(CacheHeader),
                       name.data(), h->nameLen) != 0)
                return false;
            uint32 size = b.size() - sizeof(CacheHeader) - h->nameLen;
            if (size > 0)
            {
                buffer.allocate(size);
                memcpy(buffer.data(), b.data()
                       + sizeof(CacheHeader) + h->nameLen, size);
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
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
                if (i == 0 || i == 1 || i == 3)
                    r += '/';
            }
            return r;
        }
        else
        {
            std::string a = path;
            std::string b = boost::filesystem::path(a).parent_path().string();
            std::string c = a.substr(b.length() + 1);
            if (b.empty() || c.empty())
                LOGTHROW(err2, std::runtime_error)
                        << "Cannot convert path '" << path
                        << "' into a cache path";
            return root + convertNameToPath(b, false)
                    + "/" + convertNameToPath(c, false);
        }
    }
    
    std::string stripScheme(const std::string &name)
    {
        auto p = name.find("://");
        return p == std::string::npos ? name : name.substr(p + 3);
    }

    std::string root;
    bool disabled;
    bool hashes;
};

} // namespace

Cache::~Cache()
{}

std::shared_ptr<Cache> Cache::create(const MapCreateOptions &options)
{
    return std::make_shared<CacheImpl>(options);
}

std::string convertNameToPath(std::string path, bool preserveSlashes)
{
    path = boost::filesystem::path(path).normalize().string();
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

} // namespace vts
