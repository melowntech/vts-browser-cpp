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

//#include <boost/algorithm/string.hpp>
//#include <functional> // bad_function_call exception

#include "include/vts-browser/buffer.hpp"
#include "include/vts-browser/callbacks.h"
#include "include/vts-browser/camera.h"
#include "include/vts-browser/camera.hpp"
#include "include/vts-browser/cameraCredits.hpp"
#include "include/vts-browser/cameraDraws.hpp"
#include "include/vts-browser/cameraOptions.hpp"
#include "include/vts-browser/cameraStatistics.hpp"
#include "include/vts-browser/celestial.h"
#include "include/vts-browser/celestial.hpp"
#include "include/vts-browser/exceptions.hpp"
#include "include/vts-browser/fetcher.h"
#include "include/vts-browser/fetcher.hpp"
#include "include/vts-browser/log.h"
#include "include/vts-browser/log.hpp"
#include "include/vts-browser/map.h"
#include "include/vts-browser/map.hpp"
#include "include/vts-browser/mapCallbacks.hpp"
#include "include/vts-browser/mapOptions.hpp"
#include "include/vts-browser/mapStatistics.hpp"
#include "include/vts-browser/math.h"
#include "include/vts-browser/math.hpp"
#include "include/vts-browser/navigation.h"
#include "include/vts-browser/navigation.hpp"
#include "include/vts-browser/navigationOptions.hpp"
#include "include/vts-browser/resources.h"
#include "include/vts-browser/resources.hpp"
#include "include/vts-browser/search.h"
#include "include/vts-browser/search.hpp"
#include "include/vts-browser/view.hpp"

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
    CATCH(-101, MapconfigException)
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

typedef struct vtsCNavigation
{
    std::shared_ptr<vts::Navigation> p;
} vtsCNavigation;

typedef struct vtsCSearch
{
    std::shared_ptr<vts::SearchTask> p;
} vtsCSearch;

typedef struct vtsCFetcher
{
    std::shared_ptr<vts::Fetcher> p;
} vtsCFetcher;

typedef struct vtsCDrawsGroup
{
    const std::vector<vts::DrawTask> *draws;
} vtsCDrawIterator;

typedef struct vtsCResource
{
    vts::ResourceInfo *r;
    union Ptr
    {
        vts::GpuMeshSpec *m;
        vts::GpuTextureSpec *t;
        Ptr() : m(nullptr) {}
    } ptr;
    vtsCResource() : r(nullptr) {}
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
    CATCH(-101, MapconfigException)
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

////////////////////////////////////////////////////////////////////////////
// LOG
////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////
// MAP
////////////////////////////////////////////////////////////////////////////

vtsHMap vtsMapCreate(const char *createOptions, vtsHFetcher fetcher)
{
    C_BEGIN
    vtsHMap r = new vtsCMap();
    r->p = std::make_shared<vts::Map>(vts::MapCreateOptions(createOptions),
                           fetcher ? fetcher->p : vts::Fetcher::create({}));
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

void vtsMapSetConfigPaths(vtsHMap map, const char *mapconfigPath,
                          const char *authPath)
{
    C_BEGIN
    map->p->setMapconfigPath(mapconfigPath, authPath);
    C_END
}

const char *vtsMapGetConfigPath(vtsHMap map)
{
    C_BEGIN
    return vts::retStr(map->p->getMapconfigPath());
    C_END
    return nullptr;
}

bool vtsMapGetConfigAvailable(vtsHMap map)
{
    C_BEGIN
    return map->p->getMapconfigAvailable();
    C_END
    return false;
}

bool vtsMapGetConfigReady(vtsHMap map)
{
    C_BEGIN
    return map->p->getMapconfigReady();
    C_END
    return false;
}

bool vtsMapGetProjected(vtsHMap map)
{
    C_BEGIN
    return map->p->getMapProjected();
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

void vtsMapDataInitialize(vtsHMap map)
{
    C_BEGIN
    map->p->dataInitialize();
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

void vtsMapDataAllRun(vtsHMap map)
{
    C_BEGIN
    map->p->dataAllRun();
    C_END
}

void vtsMapRenderInitialize(vtsHMap map)
{
    C_BEGIN
    map->p->renderInitialize();
    C_END
}

void vtsMapRenderTick(vtsHMap map, double elapsedTime)
{
    C_BEGIN
    map->p->renderTick(elapsedTime);
    C_END
}

void vtsMapRenderFinalize(vtsHMap map)
{
    C_BEGIN
    map->p->renderFinalize();
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

void vtsMapSetOptions(vtsHMap map, const char *options)
{
    C_BEGIN
    vts::MapRuntimeOptions mo = map->p->options();
    mo.applyJson(options);
    map->p->options() = mo; // all or nothing
    C_END
}

void vtsMapConvert(vtsHMap map,
    const double pointFrom[3], double pointTo[3],
    uint32 srsFrom, uint32 SrsTo)
{
    C_BEGIN
    map->p->convert(pointFrom, pointTo, (vts::Srs)srsFrom, (vts::Srs)SrsTo);
    C_END
}

////////////////////////////////////////////////////////////////////////////
// CAMERA
////////////////////////////////////////////////////////////////////////////

vtsHCamera vtsCameraCreate(vtsHMap map)
{
    C_BEGIN
    vtsHCamera r = new vtsCCamera();
    r->p = map->p->camera();
    return r;
    C_END
    return nullptr;
}

void vtsCameraDestroy(vtsHCamera cam)
{
    C_BEGIN
    delete cam;
    C_END
}

// camera

void vtsCameraSetViewportSize(vtsHCamera cam,
    uint32 width, uint32 height)
{
    C_BEGIN
    cam->p->setViewportSize(width, height);
    C_END
}

void vtsCameraSetView(vtsHCamera cam, const double eye[3],
    const double target[3], const double up[3])
{
    C_BEGIN
    cam->p->setView(eye, target, up);
    C_END
}

void vtsCameraSetViewMatrix(vtsHCamera cam, const double view[16])
{
    C_BEGIN
    cam->p->setView(view);
    C_END
}

void vtsCameraSetProj(vtsHCamera cam, double fovyDegs,
    double near_, double far_)
{
    C_BEGIN
    cam->p->setProj(fovyDegs, near_, far_);
    C_END
}

void vtsCameraSetProjMatrix(vtsHCamera cam, const double proj[16])
{
    C_BEGIN
    cam->p->setProj(proj);
    C_END
}

void vtsCameraGetViewportSize(vtsHCamera cam,
    uint32 *width, uint32 *height)
{
    C_BEGIN
    cam->p->getViewportSize(*width, *height);
    C_END
}

void vtsCameraGetView(vtsHCamera cam, double eye[3],
    double target[3], double up[3])
{
    C_BEGIN
    cam->p->getView(eye, target, up);
    C_END
}

void vtsCameraGetViewMatrix(vtsHCamera cam, double view[16])
{
    C_BEGIN
    cam->p->getView(view);
    C_END
}

void vtsCameraSuggestedNearFar(vtsHCamera cam,
    double *near_, double *far_)
{
    C_BEGIN
    cam->p->suggestedNearFar(*near_, *far_);
    C_END
}

// credits

const char *vtsCameraGetCredits(vtsHCamera cam)
{
    C_BEGIN
    return vts::retStr(cam->p->credits().toJson());
    C_END
    return nullptr;
}

const char *vtsCameraGetCreditsShort(vtsHCamera cam)
{
    C_BEGIN
    return vts::retStr(cam->p->credits().textShort());
    C_END
    return nullptr;
}

const char *vtsCameraGetCreditsFull(vtsHCamera cam)
{
    C_BEGIN
    return vts::retStr(cam->p->credits().textFull());
    C_END
    return nullptr;
}

// options & statistics

const char *vtsCameraGetOptions(vtsHCamera cam)
{
    C_BEGIN
    return vts::retStr(cam->p->options().toJson());
    C_END
    return nullptr;
}

const char *vtsCameraGetStatistics(vtsHCamera cam)
{
    C_BEGIN
    return vts::retStr(cam->p->statistics().toJson());
    C_END
    return nullptr;
}

void vtsCameraSetOptions(vtsHCamera cam, const char *options)
{
    C_BEGIN
    vts::CameraOptions co = cam->p->options();
    co.applyJson(options);
    cam->p->options() = co; // all or nothing
    C_END
}

////////////////////////////////////////////////////////////////////////////
// NAVIGATION
////////////////////////////////////////////////////////////////////////////

vtsHNavigation vtsNavigationCreate(vtsHCamera cam)
{
    C_BEGIN
    vtsHNavigation r = new vtsCNavigation();
    r->p = cam->p->navigation();
    return r;
    C_END
    return nullptr;
}

void vtsNavigationDestroy(vtsHNavigation nav)
{
    C_BEGIN
    delete nav;
    C_END
}

// navigation

void vtsNavigationPan(vtsHNavigation nav, const double value[3])
{
    C_BEGIN
    nav->p->pan(value);
    C_END
}

void vtsNavigationRotate(vtsHNavigation nav, const double value[3])
{
    C_BEGIN
    nav->p->rotate(value);
    C_END
}

void vtsNavigationZoom(vtsHNavigation nav, double value)
{
    C_BEGIN
    nav->p->zoom(value);
    C_END
}

void vtsNavigationResetAltitude(vtsHNavigation nav)
{
    C_BEGIN
    nav->p->resetAltitude();
    C_END
}

void vtsNavigationResetNavigationMode(vtsHNavigation nav)
{
    C_BEGIN
    nav->p->resetNavigationMode();
    C_END
}

// setters

void vtsNavigationSetSubjective(vtsHNavigation nav, bool subjective,
                                 bool convert)
{
    C_BEGIN
    nav->p->setSubjective(subjective, convert);
    C_END
}

void vtsNavigationSetPoint(vtsHNavigation nav, const double point[3])
{
    C_BEGIN
    nav->p->setPoint(point);
    C_END
}

void vtsNavigationSetRotation(vtsHNavigation nav, const double point[3])
{
    C_BEGIN
    nav->p->setRotation(point);
    C_END
}

void vtsNavigationSetViewExtent(vtsHNavigation nav, double viewExtent)
{
    C_BEGIN
    nav->p->setViewExtent(viewExtent);
    C_END
}

void vtsNavigationSetFov(vtsHNavigation nav, double fov)
{
    C_BEGIN
    nav->p->setFov(fov);
    C_END
}

void vtsNavigationSetAutoRotation(vtsHNavigation nav, double value)
{
    C_BEGIN
    nav->p->setAutoRotation(value);
    C_END
}

void vtsNavigationSetJson(vtsHNavigation nav, const char *pos)
{
    C_BEGIN
    nav->p->setPositionJson(pos);
    C_END
}

void vtsNavigationSetUrl(vtsHNavigation nav, const char *pos)
{
    C_BEGIN
    nav->p->setPositionUrl(pos);
    C_END
}

// getters

bool vtsNavigationGetSubjective(vtsHNavigation nav)
{
    C_BEGIN
    return nav->p->getSubjective();
    C_END
    return false;
}

void vtsNavigationGetPoint(vtsHNavigation nav, double point[3])
{
    C_BEGIN
    nav->p->getPoint(point);
    C_END
}

void vtsNavigationGetRotation(vtsHNavigation nav, double rot[3])
{
    C_BEGIN
    nav->p->getRotation(rot);
    C_END
}

void vtsNavigationGetRotationLimited(vtsHNavigation nav, double rot[3])
{
    C_BEGIN
    nav->p->getRotationLimited(rot);
    C_END
}

double vtsNavigationGetViewExtent(vtsHNavigation nav)
{
    C_BEGIN
    nav->p->getViewExtent();
    C_END
    return 0.0;
}

double vtsNavigationGetFov(vtsHNavigation nav)
{
    C_BEGIN
    return nav->p->getFov();
    C_END
    return 0.0;
}

double vtsNavigationGetAutoRotation(vtsHNavigation nav)
{
    C_BEGIN
    return nav->p->getAutoRotation();
    C_END
    return 0.0;
}

const char *vtsNavigationGetPositionUrl(vtsHNavigation nav)
{
    C_BEGIN
    return vts::retStr(nav->p->getPositionUrl());
    C_END
    return nullptr;
}

const char *vtsNavigationGetPositionJson(vtsHNavigation nav)
{
    C_BEGIN
    return vts::retStr(nav->p->getPositionJson());
    C_END
    return nullptr;
}

// options

const char *vtsNavigationGetOptions(vtsHNavigation nav)
{
    C_BEGIN
    return vts::retStr(nav->p->options().toJson());
    C_END
    return nullptr;
}

void vtsNavigationSetOptions(vtsHNavigation nav, const char *options)
{
    C_BEGIN
    vts::NavigationOptions no = nav->p->options();
    no.applyJson(options);
    nav->p->options() = no; // all or nothing
    C_END
}

////////////////////////////////////////////////////////////////////////////
// RESOURCES
////////////////////////////////////////////////////////////////////////////

uint32 vtsGpuTypeSize(uint32 type)
{
    C_BEGIN
    return vts::gpuTypeSize((vts::GpuTypeEnum)type);
    C_END
    return 0;
}

void vtsResourceSetUserData(vtsHResource resource, void *data,
                            vtsResourceDeleterCallbackType deleter)
{
    struct Callback
    {
        void operator()(void *data)
        {
            (*callback)(data);
        }
        vtsResourceDeleterCallbackType callback;
    };
    C_BEGIN
    Callback c;
    c.callback = deleter;
    resource->r->userData = std::shared_ptr<void>(data, c);
    C_END
}

void vtsResourceSetMemoryCost(vtsHResource resource,
                              uint32 ramMem, uint32 gpuMem)
{
    C_BEGIN
    resource->r->ramMemoryCost = ramMem;
    resource->r->gpuMemoryCost = gpuMem;
    C_END
}

void vtsTextureGetResolution(vtsHResource resource,
        uint32 *width, uint32 *height, uint32 *components)
{
    C_BEGIN
    *width = resource->ptr.t->width;
    *height = resource->ptr.t->height;
    *components = resource->ptr.t->components;
    C_END
}

uint32 vtsTextureGetType(vtsHResource resource)
{
    C_BEGIN
    return (uint32)resource->ptr.t->type;
    C_END
    return 0;
}

uint32 vtsTextureGetInternalFormat(vtsHResource resource)
{
    C_BEGIN
    return resource->ptr.t->internalFormat;
    C_END
    return 0;
}

uint32 vtsTextureGetFilterMode(vtsHResource resource)
{
    C_BEGIN
    return (uint32)resource->ptr.t->filterMode;
    C_END
    return 0;
}

uint32 vtsTextureGetWrapMode(vtsHResource resource)
{
    C_BEGIN
    return (uint32)resource->ptr.t->wrapMode;
    C_END
    return 0;
}

void vtsTextureGetBuffer(vtsHResource resource,
        void **data, uint32 *size)
{
    C_BEGIN
    *data = resource->ptr.t->buffer.data();
    *size = resource->ptr.t->buffer.size();
    C_END
}

uint32 vtsMeshGetFaceMode(vtsHResource resource)
{
    C_BEGIN
    return (uint32)resource->ptr.m->faceMode;
    C_END
    return 0;
}

void vtsMeshGetVertices(vtsHResource resource,
        void **data, uint32 *size, uint32 *count)
{
    C_BEGIN
    *data = resource->ptr.m->vertices.data();
    *size = resource->ptr.m->vertices.size();
    *count = resource->ptr.m->verticesCount;
    C_END
}

void vtsMeshGetIndices(vtsHResource resource,
        void **data, uint32 *size, uint32 *count)
{
    C_BEGIN
    *data = resource->ptr.m->indices.data();
    *size = resource->ptr.m->indices.size();
    *count = resource->ptr.m->indicesCount;
    C_END
}

void vtsMeshGetAttribute(vtsHResource resource, uint32 index,
        uint32 *offset, uint32 *stride, uint32 *components,
        uint32 *type, bool *enable, bool *normalized)
{
    C_BEGIN
    if (index >= resource->ptr.m->attributes.size())
    {
        *offset = 0;
        *stride = 0;
        *components = 0;
        *type = 0;
        *enable = false;
        *normalized = false;
    }
    else
    {
        const vts::GpuMeshSpec::VertexAttribute &a
            = resource->ptr.m->attributes[index];
        *offset = a.offset;
        *stride = a.stride;
        *components = a.components;
        *type = (uint32)a.type;
        *enable = a.enable;
        *normalized = a.normalized;
    }
    C_END
}

////////////////////////////////////////////////////////////////////////////
// CALLBACKS
////////////////////////////////////////////////////////////////////////////

namespace
{

struct StateCallback
{
    void operator()()
    {
        (*callback)(map);
    }
    vtsHMap map;
    vtsEmptyCallbackType callback;
};

} // namespace

void vtsCallbacksLoadTexture(vtsHMap map,
                vtsResourceCallbackType callback)
{
    struct Callback
    {
        void operator()(vts::ResourceInfo &r, vts::GpuTextureSpec &t)
        {
            vtsCResource rc;
            rc.r = &r;
            rc.ptr.t = &t;
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
            rc.ptr.m = &m;
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

void vtsCallbacksMapconfigAvailable(vtsHMap map,
                vtsEmptyCallbackType callback)
{
    C_BEGIN
    StateCallback c;
    c.map = map;
    c.callback = callback;
    map->p->callbacks().mapconfigAvailable = c;
    C_END
}

void vtsCallbacksMapconfigReady(vtsHMap map,
                vtsEmptyCallbackType callback)
{
    C_BEGIN
    StateCallback c;
    c.map = map;
    c.callback = callback;
    map->p->callbacks().mapconfigReady = c;
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

////////////////////////////////////////////////////////////////////////////
// CELESTIAL
////////////////////////////////////////////////////////////////////////////

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

// colors: horizon rgba, zenith rgba
// parameters: color gradient exponent,
//   thickness, quantile, visibility, quantile
void vtsCelestialAtmosphere(vtsHMap map, float colors[8], double params[5])
{
    C_BEGIN
    const auto &a = map->p->celestialBody().atmosphere;
    if (colors)
    {
        for (int i = 0; i < 4; i++)
            colors[i] = a.colorHorizon[i];
        for (int i = 0; i < 4; i++)
            colors[i + 4] = a.colorZenith[i];
    }
    if (params)
    {
        params[0] = a.colorGradientExponent;
        params[1] = a.thickness;
        params[2] = a.thicknessQuantile;
        params[3] = a.visibility;
        params[4] = a.visibilityQuantile;
    }
    C_END
}

void vtsCelestialAtmosphereDerivedAttributes(vtsHMap map,
    double *boundaryThickness,
    double *horizontalExponent, double *verticalExponent)
{
    C_BEGIN
    const auto &a = map->p->celestialBody();
    double bt, he, ve;
    atmosphereDerivedAttributes(a, bt, he, ve);
    if (boundaryThickness)
        *boundaryThickness = bt;
    if (horizontalExponent)
        *horizontalExponent = he;
    if (verticalExponent)
        *verticalExponent = ve;
    C_END
}

////////////////////////////////////////////////////////////////////////////
// DRAWS
////////////////////////////////////////////////////////////////////////////

namespace
{

vtsHDrawsGroup createDrawIterator(std::vector<vts::DrawTask> &vec)
{
    if (vec.empty())
        return nullptr;
    vtsHDrawsGroup r = new vtsCDrawsGroup;
    r->draws = &vec;
    return r;
}

} // namespace

vtsHDrawsGroup vtsDrawsOpaque(vtsHCamera cam)
{
    C_BEGIN
    return createDrawIterator(cam->p->draws().opaque);
    C_END
    return nullptr;
}

vtsHDrawsGroup vtsDrawsTransparent(vtsHCamera cam)
{
    C_BEGIN
    return createDrawIterator(cam->p->draws().transparent);
    C_END
    return nullptr;
}

vtsHDrawsGroup vtsDrawsGeodata(vtsHCamera cam)
{
    C_BEGIN
    return createDrawIterator(cam->p->draws().geodata);
    C_END
    return nullptr;
}

vtsHDrawsGroup vtsDrawsInfographics(vtsHCamera cam)
{
    C_BEGIN
    return createDrawIterator(cam->p->draws().infographics);
    C_END
    return nullptr;
}

vtsHDrawsGroup vtsDrawsColliders(vtsHCamera cam)
{
    C_BEGIN
    return createDrawIterator(cam->p->draws().colliders);
    C_END
    return nullptr;
}

uint32 vtsDrawsCount(vtsHDrawsGroup group)
{
    C_BEGIN
    if (group)
        return group->draws->size();
    else
        return 0;
    C_END
    return 0;
}

void vtsDrawsDestroy(vtsHDrawsGroup group)
{
    C_BEGIN
    delete group;
    C_END
}

void *vtsDrawsMesh(vtsHDrawsGroup group, uint32 index)
{
    C_BEGIN
    return (*group->draws)[index].mesh.get();
    C_END
    return nullptr;
}

void *vtsDrawsTexColor(vtsHDrawsGroup group, uint32 index)
{
    C_BEGIN
    return (*group->draws)[index].texColor.get();
    C_END
    return nullptr;
}

void *vtsDrawsTexMask(vtsHDrawsGroup group, uint32 index)
{
    C_BEGIN
    return (*group->draws)[index].texMask.get();
    C_END
    return nullptr;
}

const vtsCDrawBase *vtsDrawsDetail(vtsHDrawsGroup group, uint32 index)
{
    C_BEGIN
    return &(*group->draws)[index];
    C_END
    return nullptr;
}

const vtsCDrawBase *vtsDrawsAllInOne(vtsHDrawsGroup group, uint32 index,
                              void **mesh, void **texColor, void **texMask)
{
    C_BEGIN
    const vts::DrawTask &t = (*group->draws)[index];
    *mesh = t.mesh.get();
    *texColor = t.texColor.get();
    *texMask = t.texMask.get();
    return &t;
    C_END
    return nullptr;
}

const vtsCCameraBase *vtsDrawsCamera(vtsHCamera cam)
{
    C_BEGIN
    return &cam->p->draws().camera;
    C_END
    return nullptr;
}

void *vtsDrawsAtmosphereDensityTexture(vtsHMap map)
{
    C_BEGIN
    return map->p->atmosphereDensityTexture().get();
    C_END
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
// FETCHER
////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////
// SEARCHING
////////////////////////////////////////////////////////////////////////////

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

bool vtsSearchGetDone(vtsHSearch search)
{
    C_BEGIN
    return search->p->done;
    C_END
    return false;
}

uint32 vtsSearchGetResultsCount(vtsHSearch search)
{
    assert((bool)search->p->done);
    C_BEGIN
    return search->p->results.size();
    C_END
    return 0;
}

const char *vtsSearchGetResultData(vtsHSearch search, uint32 index)
{
    assert((bool)search->p->done);
    C_BEGIN
    return vts::retStr(search->p->results[index].toJson());
    C_END
    return nullptr;
}

void vtsSearchUpdateDistances(vtsHSearch search,
                const double point[3])
{
    C_BEGIN
    search->p->updateDistances(point);
    C_END
}

////////////////////////////////////////////////////////////////////////////
// MATH
////////////////////////////////////////////////////////////////////////////

void vtsMathMul44x44(double result[16], const double l[16],
    const double r[16])
{
    C_BEGIN
    vts::matToRaw(vts::mat4(vts::rawToMat4(l) * vts::rawToMat4(r)), result);
    C_END
}

void vtsMathMul33x33(double result[9], const double l[9],
    const double r[9])
{
    C_BEGIN
    vts::matToRaw(vts::mat3(vts::rawToMat3(l) * vts::rawToMat3(r)), result);
    C_END
}

void vtsMathMul44x4(double result[4], const double l[16],
    const double r[4])
{
    C_BEGIN
    vts::vecToRaw(vts::vec4(vts::rawToMat4(l) * vts::rawToVec4(r)), result);
    C_END
}

void vtsMathInverse44(double result[16], const double r[16])
{
    C_BEGIN
    vts::matToRaw(vts::mat4(vts::rawToMat4(r).inverse()), result);
    C_END
}

void vtsMathInverse33(double result[9], const double r[9])
{
    C_BEGIN
    vts::matToRaw(vts::mat3(vts::rawToMat3(r).inverse()), result);
    C_END
}

#ifdef __cplusplus
} // extern C
#endif


