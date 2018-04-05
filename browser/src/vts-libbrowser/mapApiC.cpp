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

#include <boost/algorithm/string.hpp>

#include "include/vts-browser/callbacks.h"
#include "include/vts-browser/celestial.h"
#include "include/vts-browser/draws.h"
#include "include/vts-browser/fetcher.h"
#include "include/vts-browser/log.h"
#include "include/vts-browser/map.h"
#include "include/vts-browser/resources.h"
#include "include/vts-browser/search.h"

#include "include/vts-browser/map.hpp"
#include "include/vts-browser/exceptions.hpp"
#include "include/vts-browser/log.hpp"
#include "include/vts-browser/options.hpp"
#include "include/vts-browser/statistics.hpp"
#include "include/vts-browser/search.hpp"
#include "include/vts-browser/credits.hpp"
#include "include/vts-browser/celestial.hpp"
#include "include/vts-browser/callbacks.hpp"
#include "include/vts-browser/resources.hpp"
#include "include/vts-browser/fetcher.hpp"
#include "include/vts-browser/draws.hpp"

#include "utilities/json.hpp"
#include "mapApiC.hpp"

namespace vts
{

namespace
{

struct TlsState
{
    std::string str;
    std::string msg;
    sint32 code;

    TlsState() : code(0) {}
};

thread_local TlsState tlsState;

} // namespace

void handleExceptions()
{
    try
    {
        throw;
    }
#define CATCH(C, E) catch (E &e) { setError(C, e.what()); }
    CATCH(-101, MapConfigException)
    CATCH(-100, AuthException)
    CATCH(-21, std::ios_base::failure)
    CATCH(-20, std::invalid_argument)
    CATCH(-19, std::domain_error)
    CATCH(-18, std::length_error)
    CATCH(-17, std::out_of_range)
    //CATCH(-16, std::future_error)
    CATCH(-15, std::range_error)
    CATCH(-14, std::overflow_error)
    CATCH(-13, std::underflow_error)
    //CATCH(-12, std::regex_error)
    CATCH(-11, std::system_error)
    CATCH(-10, std::bad_array_new_length)
    CATCH(-9, std::logic_error)
    CATCH(-8, std::runtime_error)
    CATCH(-7, std::bad_typeid)
    CATCH(-6, std::bad_cast)
    CATCH(-5, std::bad_weak_ptr)
    CATCH(-4, std::bad_function_call)
    CATCH(-3, std::bad_alloc)
    CATCH(-2, std::bad_exception)
    CATCH(-1, std::exception)
#undef CATCH
    catch (const char *e) { setError(-999, e); }
    catch (...) { setError(-1000, "unknown exception"); }
}

void setError(sint32 code, const std::string &msg)
{
    TlsState &s = tlsState;
    s.code = code;
    s.msg = msg;
}

const char *retStr(const std::string &str)
{
    TlsState &s = tlsState;
    s.str = str;
    return s.str.c_str();
}

} // namespace vts

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vtsCSearch
{
    std::shared_ptr<vts::SearchTask> p;
} vtsCSearch;

typedef struct vtsCFetcher
{
    std::shared_ptr<vts::Fetcher> p;
} vtsCFetcher;

typedef struct vtsCDrawIterator
{
    std::vector<vts::DrawTask>::iterator it, et;
} vtsCDrawIterator;

typedef struct vtsCResource
{
    vts::ResourceInfo *r;
    vts::GpuMeshSpec *m;
    vts::GpuTextureSpec *t;
    vtsCResource() : r(nullptr), m(nullptr), t(nullptr) {}
} vtsCResource;

sint32 vtsErrCode()
{
    vts::TlsState &s = vts::tlsState;
    return s.code;
}

const char *vtsErrMsg()
{
    vts::TlsState &s = vts::tlsState;
    return s.msg.c_str();
}

const char *vtsErrCodeToName(sint32 code)
{
    switch (code)
    {
    case 0: return "no error";
#define CATCH(C, E) case C: return #E;
    CATCH(-101, MapConfigException)
    CATCH(-100, AuthException)
    CATCH(-21, std::ios_base::failure)
    CATCH(-20, std::invalid_argument)
    CATCH(-19, std::domain_error)
    CATCH(-18, std::length_error)
    CATCH(-17, std::out_of_range)
    //CATCH(-16, std::future_error)
    CATCH(-15, std::range_error)
    CATCH(-14, std::overflow_error)
    CATCH(-13, std::underflow_error)
    //CATCH(-12, std::regex_error)
    CATCH(-11, std::system_error)
    CATCH(-10, std::bad_array_new_length)
    CATCH(-9, std::logic_error)
    CATCH(-8, std::runtime_error)
    CATCH(-7, std::bad_typeid)
    CATCH(-6, std::bad_cast)
    CATCH(-5, std::bad_weak_ptr)
    CATCH(-4, std::bad_function_call)
    CATCH(-3, std::bad_alloc)
    CATCH(-2, std::bad_exception)
    CATCH(-1, std::exception)
#undef CATCH
    case -999: return "literal exception";
    case -1000: return "unknown exception";
    default: return "";
    }
}

void vtsErrClear()
{
    vts::TlsState &s = vts::tlsState;
    s.code = 0;
    s.msg = "";
}

void vtsLogSetMaskStr(const char *mask)
{
    C_BEGIN
    vts::setLogMask(std::string(mask));
    C_END
}

void vtsLogSetMaskCode(uint32 mask)
{
    C_BEGIN
    vts::setLogMask((vts::LogLevel)mask);
    C_END
}

void vtsLogSetConsole(bool enable)
{
    C_BEGIN
    vts::setLogConsole(enable);
    C_END
}

void vtsLogSetFile(const char *filename)
{
    C_BEGIN
    vts::setLogFile(filename);
    C_END
}

void vtsLogSetThreadName(const char *name)
{
    C_BEGIN
    vts::setLogThreadName(name);
    C_END
}

void vtsLogAddSink(uint32 mask, vtsLogCallbackType callback)
{
    struct Callback
    {
        void operator()(const std::string &msg)
        {
            (*callback)(msg.c_str());
        }
        vtsLogCallbackType callback;
    };
    C_BEGIN
    Callback sink;
    sink.callback = callback;
    vts::addLogSink((vts::LogLevel)mask, sink);
    C_END
}

void vtsLog(uint32 level, const char *message)
{
    try
    {
        vts::log((vts::LogLevel)level, message);
    }
    catch (...)
    {
        // silently ignore
    }
}

vtsHMap vtsMapCreate(const char *createOptions)
{
    C_BEGIN
    vtsHMap r = new vtsCMap();
    r->p = std::make_shared<vts::Map>(vts::MapCreateOptions(createOptions));
    return r;
    C_END
    return nullptr;
}

void vtsMapDestroy(vtsHMap map)
{
    C_BEGIN
    delete map;
    C_END
}

void vtsMapSetCustomData(vtsHMap map, void *data)
{
    C_BEGIN
    map->userData = data;
    C_END
}

void *vtsMapGetCustomData(vtsHMap map)
{
    C_BEGIN
    return map->userData;
    C_END
    return nullptr;
}

void vtsMapSetConfigPaths(vtsHMap map, const char *mapConfigPath,
                          const char *authPath, const char *sriPath)
{
    C_BEGIN
    map->p->setMapConfigPath(mapConfigPath, authPath, sriPath);
    C_END
}

const char *vtsMapGetConfigPath(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->getMapConfigPath());
    C_END
    return nullptr;
}

bool vtsMapGetConfigAvailable(vtsHMap map)
{
    C_BEGIN
    return map->p->getMapConfigAvailable();
    C_END
    return false;
}

bool vtsMapGetConfigReady(vtsHMap map)
{
    C_BEGIN
    return map->p->getMapConfigReady();
    C_END
    return false;
}

bool vtsMapGetRenderComplete(vtsHMap map)
{
    C_BEGIN
    return map->p->getMapRenderComplete();
    C_END
    return false;
}

double vtsMapGetRenderProgress(vtsHMap map)
{
    C_BEGIN
    return map->p->getMapRenderProgress();
    C_END
    return 0.0;
}

void vtsMapDataInitialize(vtsHMap map, vtsHFetcher fetcher)
{
    C_BEGIN
    map->p->dataInitialize(fetcher ? fetcher->p
                                   : vts::Fetcher::create({}));
    C_END
}

void vtsMapDataTick(vtsHMap map)
{
    C_BEGIN
    map->p->dataTick();
    C_END
}

void vtsMapDataFinalize(vtsHMap map)
{
    C_BEGIN
    map->p->dataFinalize();
    C_END
}

void vtsMapRenderInitialize(vtsHMap map)
{
    C_BEGIN
    map->p->renderInitialize();
    C_END
}

void vtsMapRenderTickPrepare(vtsHMap map, double elapsedTime)
{
    C_BEGIN
    map->p->renderTickPrepare(elapsedTime);
    C_END
}

void vtsMapRenderTickRender(vtsHMap map)
{
    C_BEGIN
    map->p->renderTickRender();
    C_END
}

void vtsMapRenderFinalize(vtsHMap map)
{
    C_BEGIN
    map->p->renderFinalize();
    C_END
}

void vtsMapSetWindowSize(vtsHMap map, uint32 width, uint32 height)
{
    C_BEGIN
    map->p->setWindowSize(width, height);
    C_END
}

const char *vtsMapGetOptions(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->options().toJson());
    C_END
    return nullptr;
}

const char *vtsMapGetStatistics(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->statistics().toJson());
    C_END
    return nullptr;
}

const char *vtsMapGetCredits(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->credits().toJson());
    C_END
    return nullptr;
}

const char *vtsMapGetCreditsShort(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->credits().textShort());
    C_END
    return nullptr;
}

const char *vtsMapGetCreditsFull(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->credits().textFull());
    C_END
    return nullptr;
}

void vtsMapSetOptions(vtsHMap map, const char *options)
{
    C_BEGIN
    map->p->options() = vts::MapOptions(options);
    C_END
}

void vtsMapPan(vtsHMap map, const double value[3])
{
    C_BEGIN
    map->p->pan(value);
    C_END
}

void vtsMapRotate(vtsHMap map, const double value[3])
{
    C_BEGIN
    map->p->rotate(value);
    C_END
}

void vtsMapZoom(vtsHMap map, double value)
{
    C_BEGIN
    map->p->zoom(value);
    C_END
}

void vtsMapResetPositionAltitude(vtsHMap map)
{
    C_BEGIN
    map->p->resetPositionAltitude();
    C_END
}

void vtsMapResetNavigationMode(vtsHMap map)
{
    C_BEGIN
    map->p->resetNavigationMode();
    C_END
}

void vtsMapSetPositionSubjective(vtsHMap map, bool subjective,
                                 bool convert)
{
    C_BEGIN
    map->p->setPositionSubjective(subjective, convert);
    C_END
}

void vtsMapSetPositionPoint(vtsHMap map, const double point[3])
{
    C_BEGIN
    map->p->setPositionPoint(point);
    C_END
}

void vtsMapSetPositionRotation(vtsHMap map, const double point[3])
{
    C_BEGIN
    map->p->setPositionRotation(point);
    C_END
}

void vtsMapSetPositionViewExtent(vtsHMap map, double viewExtent)
{
    C_BEGIN
    map->p->setPositionViewExtent(viewExtent);
    C_END
}

void vtsMapSetPositionFov(vtsHMap map, double fov)
{
    C_BEGIN
    map->p->setPositionFov(fov);
    C_END
}

void vtsMapSetPositionJson(vtsHMap map, const char *position)
{
    C_BEGIN
    map->p->setPositionJson(position);
    C_END
}

void vtsMapSetPositionUrl(vtsHMap map, const char *position)
{
    C_BEGIN
    map->p->setPositionUrl(position);
    C_END
}

void vtsMapSetAutoRotation(vtsHMap map, double value)
{
    C_BEGIN
    map->p->setAutoRotation(value);
    C_END
}

bool vtsMapGetPositionSubjective(vtsHMap map)
{
    C_BEGIN
    return map->p->getPositionSubjective();
    C_END
    return false;
}

void vtsMapGetPositionPoint(vtsHMap map, double point[3])
{
    C_BEGIN
    map->p->getPositionPoint(point);
    C_END
}

void vtsMapGetPositionRotation(vtsHMap map, double rot[3])
{
    C_BEGIN
    map->p->getPositionRotation(rot);
    C_END
}

void vtsMapGetPositionRotationLimited(vtsHMap map, double rot[3])
{
    C_BEGIN
    map->p->getPositionRotationLimited(rot);
    C_END
}

double vtsMapGetPositionViewExtent(vtsHMap map)
{
    C_BEGIN
    map->p->getPositionViewExtent();
    C_END
    return 0.0;
}

double vtsMapGetPositionFov(vtsHMap map)
{
    C_BEGIN
    return map->p->getPositionFov();
    C_END
    return 0.0;
}

const char *vtsMapGetPositionUrl(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->getPositionUrl());
    C_END
    return nullptr;
}

const char *vtsMapGetPositionJson(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->getPositionJson());
    C_END
    return nullptr;
}

double vtsMapGetAutoRotation(vtsHMap map)
{
    C_BEGIN
    return map->p->getAutoRotation();
    C_END
    return 0.0;
}

void vtsMapConvert(vtsHMap map,
                   const double pointFrom[3], double pointTo[3],
                   uint32 srsFrom, uint32 SrsTo)
{
    C_BEGIN
    map->p->convert(pointFrom, pointTo, (vts::Srs)srsFrom, (vts::Srs)SrsTo);
    C_END
}

uint32 vtsGpuTypeSize(uint32 type)
{
    C_BEGIN
    return vts::gpuTypeSize((vts::GpuTypeEnum)type);
    C_END
    return 0;
}

void vtsSetResourceUserData(vtsHResource resource, void *data,
                            vtsResourceUserDeleterType deleter)
{
    struct Callback
    {
        void operator()(void *data)
        {
            (*callback)(data);
        }
        vtsResourceUserDeleterType callback;
    };
    C_BEGIN
    Callback c;
    c.callback = deleter;
    resource->r->userData = std::shared_ptr<void>(data, c);
    C_END
}

void vtsSetResourceMemoryCost(vtsHResource resource,
                              uint32 ramMem, uint32 gpuMem)
{
    C_BEGIN
    resource->r->ramMemoryCost = ramMem;
    resource->r->gpuMemoryCost = gpuMem;
    C_END
}

void vtsGetTextureResolution(vtsHResource resource,
        uint32 *width, uint32 *height, uint32 *components)
{
    C_BEGIN
    *width = resource->t->width;
    *height = resource->t->height;
    *components = resource->t->components;
    C_END
}

uint32 vtsGetTextureType(vtsHResource resource)
{
    C_BEGIN
    return (uint32)resource->t->type;
    C_END
    return 0;
}

uint32 vtsGetTextureInternalFormat(vtsHResource resource)
{
    C_BEGIN
    return resource->t->internalFormat;
    C_END
    return 0;
}

void vtsGetTextureBuffer(vtsHResource resource,
        void **data, uint32 *size)
{
    C_BEGIN
    *data = resource->t->buffer.data();
    *size = resource->t->buffer.size();
    C_END
}

uint32 vtsGetMeshFaceMode(vtsHResource resource)
{
    C_BEGIN
    return (uint32)resource->m->faceMode;
    C_END
    return 0;
}

void vtsGetMeshVertices(vtsHResource resource,
        void **data, uint32 *size, uint32 *count)
{
    C_BEGIN
    *data = resource->m->vertices.data();
    *size = resource->m->vertices.size();
    *count = resource->m->verticesCount;
    C_END
}

void vtsGetMeshIndices(vtsHResource resource,
        void **data, uint32 *size, uint32 *count)
{
    C_BEGIN
    *data = resource->m->indices.data();
    *size = resource->m->indices.size();
    *count = resource->m->indicesCount;
    C_END
}

void vtsGetMeshAttribute(vtsHResource resource, uint32 index,
        uint32 *offset, uint32 *stride, uint32 *components,
        uint32 *type, bool *enable, bool *normalized)
{
    C_BEGIN
    const vts::GpuMeshSpec::VertexAttribute &a
            = resource->m->attributes[index];
    *offset = a.offset;
    *stride = a.stride;
    *components = a.components;
    *type = (uint32)a.type;
    *enable = a.enable;
    *normalized = a.normalized;
    C_END
}

void vtsCallbacksLoadTexture(vtsHMap map,
                vtsResourceCallbackType callback)
{
    struct Callback
    {
        void operator()(vts::ResourceInfo &r, vts::GpuTextureSpec &t)
        {
            vtsCResource rc;
            rc.r = &r;
            rc.t = &t;
            (*callback)(map, &rc);
        }
        vtsHMap map;
        vtsResourceCallbackType callback;
    };
    C_BEGIN
    Callback c;
    c.map = map;
    c.callback = callback;
    map->p->callbacks().loadTexture = c;
    C_END
}

void vtsCallbacksLoadMesh(vtsHMap map,
                vtsResourceCallbackType callback)
{
    struct Callback
    {
        void operator()(vts::ResourceInfo &r, vts::GpuMeshSpec &m)
        {
            vtsCResource rc;
            rc.r = &r;
            rc.m = &m;
            (*callback)(map, &rc);
        }
        vtsHMap map;
        vtsResourceCallbackType callback;
    };
    C_BEGIN
    Callback c;
    c.map = map;
    c.callback = callback;
    map->p->callbacks().loadMesh = c;
    C_END
}


namespace
{

struct StateCallback
{
    void operator()()
    {
        (*callback)(map);
    }
    vtsHMap map;
    vtsStateCallbackType callback;
};

} // namespace

void vtsCallbacksMapconfigAvailable(vtsHMap map,
                vtsStateCallbackType callback)
{
    C_BEGIN
    StateCallback c;
    c.map = map;
    c.callback = callback;
    map->p->callbacks().mapconfigAvailable = c;
    C_END
}

void vtsCallbacksMapconfigReady(vtsHMap map,
                vtsStateCallbackType callback)
{
    C_BEGIN
    StateCallback c;
    c.map = map;
    c.callback = callback;
    map->p->callbacks().mapconfigReady = c;
    C_END
}

namespace
{

struct CameraCallback
{
    void operator()(double *ds)
    {
        (*callback)(map, ds);
    }
    vtsHMap map;
    vtsCameraOverrideCallbackType callback;
    CameraCallback(vtsHMap map,
                vtsCameraOverrideCallbackType callback)
        : map(map), callback(callback) {}
};

} // namespace

void vtsCallbacksCameraEye(vtsHMap map,
                vtsCameraOverrideCallbackType callback)
{
    C_BEGIN
    map->p->callbacks().cameraOverrideEye = CameraCallback (map, callback);
    C_END
}

void vtsCallbacksCameraTarget(vtsHMap map,
                vtsCameraOverrideCallbackType callback)
{
    C_BEGIN
    map->p->callbacks().cameraOverrideTarget = CameraCallback (map, callback);
    C_END
}

void vtsCallbacksCameraUp(vtsHMap map,
                vtsCameraOverrideCallbackType callback)
{
    C_BEGIN
    map->p->callbacks().cameraOverrideUp = CameraCallback (map, callback);
    C_END
}

void vtsCallbacksCameraFovAspectNearFar(vtsHMap map,
                vtsCameraOverrideCallbackType callback)
{
    struct Callback
    {
        void operator()(double &a, double &b, double &c, double &d)
        {
            double tmp[4] = {a, b, c, d};
            (*callback)(map, tmp);
            a = tmp[0];
            b = tmp[1];
            c = tmp[2];
            d = tmp[3];
        }
        vtsHMap map;
        vtsCameraOverrideCallbackType callback;
    };
    C_BEGIN
    Callback c;
    c.map = map;
    c.callback = callback;
    map->p->callbacks().cameraOverrideFovAspectNearFar = c;
    C_END
}

void vtsCallbacksCameraView(vtsHMap map,
                vtsCameraOverrideCallbackType callback)
{
    C_BEGIN
    map->p->callbacks().cameraOverrideView = CameraCallback (map, callback);
    C_END
}

void vtsCallbacksCameraProj(vtsHMap map,
                vtsCameraOverrideCallbackType callback)
{
    C_BEGIN
    map->p->callbacks().cameraOverrideProj = CameraCallback (map, callback);
    C_END
}

void vtsProjFinder(vtsProjFinderCallbackType callback)
{
    struct Callback
    {
        const char *operator()(const std::string &msg)
        {
            return (*callback)(msg.c_str());
        }
        vtsProjFinderCallbackType callback;
    };
    C_BEGIN
    Callback c;
    c.callback = callback;
    vts::projFinder = c;
    C_END
}

const char *vtsCelestialName(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->celestialBody().name);
    C_END
    return nullptr;
}

double vtsCelestialMajorRadius(vtsHMap map)
{
    C_BEGIN
    return map->p->celestialBody().majorRadius;
    C_END
    return 0.0;
}

double vtsCelestialMinorRadius(vtsHMap map)
{
    C_BEGIN
    return map->p->celestialBody().minorRadius;
    C_END
    return 0.0;
}

double vtsCelestialAtmosphereThickness(vtsHMap map)
{
    C_BEGIN
    return map->p->celestialBody().atmosphere.thickness;
    C_END
    return 0.0;
}

const vtsCCameraBase *vtsDrawsCamera(vtsHMap map)
{
    C_BEGIN
    return &map->p->draws().camera;
    C_END
    return nullptr;
}

namespace
{

vtsHDrawIterator createDrawIterator(std::vector<vts::DrawTask> &vec)
{
    if (vec.empty())
        return nullptr;
    vtsHDrawIterator r = new vtsCDrawIterator;
    r->it = vec.begin();
    r->et = vec.end();
    return r;
}

} // namespace

vtsHDrawIterator vtsDrawsOpaque(vtsHMap map)
{
    C_BEGIN
    return createDrawIterator(map->p->draws().opaque);
    C_END
    return nullptr;
}

vtsHDrawIterator vtsDrawsTransparent(vtsHMap map)
{
    C_BEGIN
    return createDrawIterator(map->p->draws().transparent);
    C_END
    return nullptr;
}

vtsHDrawIterator vtsDrawsGeodata(vtsHMap map)
{
    C_BEGIN
    return createDrawIterator(map->p->draws().geodata);
    C_END
    return nullptr;
}

vtsHDrawIterator vtsDrawsInfographic(vtsHMap map)
{
    C_BEGIN
    return createDrawIterator(map->p->draws().Infographic);
    C_END
    return nullptr;
}

bool vtsDrawsNext(vtsHDrawIterator iterator)
{
    C_BEGIN
    if (iterator->it == iterator->et)
        return false;
    iterator->it++;
    return iterator->it != iterator->et;
    C_END
    return false;
}

void vtsDrawsDestroy(vtsHDrawIterator iterator)
{
    C_BEGIN
    delete iterator;
    C_END
}

void *vtsDrawsMesh(vtsHDrawIterator iterator)
{
    C_BEGIN
    return iterator->it->mesh.get();
    C_END
    return nullptr;
}

void *vtsDrawsTexColor(vtsHDrawIterator iterator)
{
    C_BEGIN
    return iterator->it->texColor.get();
    C_END
    return nullptr;
}

void *vtsDrawsTexMask(vtsHDrawIterator iterator)
{
    C_BEGIN
    return iterator->it->texMask.get();
    C_END
    return nullptr;
}

const vtsCDrawBase *vtsDrawsDetail(vtsHDrawIterator iterator)
{
    C_BEGIN
    return &*iterator->it;
    C_END
    return nullptr;
}

vtsHFetcher vtsFetcherCreateDefault(const char *createOptions)
{
    C_BEGIN
    vtsHFetcher r = new vtsCFetcher();
    r->p = vts::Fetcher::create(vts::FetcherOptions(createOptions));
    return r;
    C_END
    return nullptr;
}

void vtsFetcherDestroy(vtsHFetcher fetcher)
{
    C_BEGIN
    delete fetcher;
    C_END
}

bool vtsMapGetSearchable(vtsHMap map)
{
    C_BEGIN
    return map->p->searchable();
    C_END
    return false;
}

vtsHSearch vtsMapSearch(vtsHMap map, const char *query)
{
    C_BEGIN
    vtsHSearch r = new vtsCSearch();
    r->p = map->p->search(query);
    return r;
    C_END
    return nullptr;
}

vtsHSearch vtsMapSearchAt(vtsHMap map, const char *query,
                          const double point[3])
{
    C_BEGIN
    vtsHSearch r = new vtsCSearch();
    r->p = map->p->search(query, point);
    return r;
    C_END
    return nullptr;
}

void vtsSearchDestroy(vtsHSearch search)
{
    C_BEGIN
    delete search;
    C_END
}

bool vtsSearchDone(vtsHSearch search)
{
    C_BEGIN
    return search->p->done;
    C_END
    return false;
}

uint32 vtsSearchResultsCount(vtsHSearch search)
{
    assert((bool)search->p->done);
    C_BEGIN
    return search->p->results.size();
    C_END
    return 0;
}

void vtsSearchUpdateDistances(vtsHSearch search,
                const double point[3])
{
    C_BEGIN
    search->p->updateDistances(point);
    C_END
}

const char *vtsSearchDisplayName(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].displayName);
    C_END
    return nullptr;
}

const char *vtsSearchTitle(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].title);
    C_END
    return nullptr;
}

const char *vtsSearchType(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].type);
    C_END
    return nullptr;
}

const char *vtsSearchRegion(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].region);
    C_END
    return nullptr;
}

const char *vtsSearchRoad(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].road);
    C_END
    return nullptr;
}

const char *vtsSearchCity(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].city);
    C_END
    return nullptr;
}

const char *vtsSearchCounty(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].county);
    C_END
    return nullptr;
}

const char *vtsSearchState(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].state);
    C_END
    return nullptr;
}

const char *vtsSearchHouseNumber(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].houseNumber);
    C_END
    return nullptr;
}

const char *vtsSearchStateDistrict(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].stateDistrict);
    C_END
    return nullptr;
}

const char *vtsSearchCountry(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].country);
    C_END
    return nullptr;
}

const char *vtsSearchCountryCode(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].countryCode);
    C_END
    return nullptr;
}

void vtsSearchPosition(vtsHSearch search, uint32 index,
                double point[3])
{
    assert((bool)search->p->done);
    C_BEGIN
    auto &r = search->p->results[index].position;
    for (int i = 0; i < 3; i++)
        point[i] = r[i];
    C_END
}

double vtsSearchRadius(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return search->p->results[index].radius;
    C_END
    return 0.0;
}

double vtsSearchDistance(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return search->p->results[index].distance;
    C_END
    return 0.0;
}

double vtsSearchImportance(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return search->p->results[index].importance;
    C_END
    return 0.0;
}

#ifdef __cplusplus
} // extern C
#endif


