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


// this example app is intended as a very ROUGH test
//   that the vts-browser.dll is working.
// it should be considered highly experimental
// ERRORS ARE TO BE EXPECTED
// bugfixes are always welcome


// uwp window management code based on
//   http://ysflight.in.coocan.jp/programming/uwp_gles2_cmake/e.html


// if the compiler cannot find some of these headers
//   make sure to add nuget package: ANGLE.WindowsStore
// it has to be added again every time cmake is rerun
//   (eg. change the version)
//   to update the reference in the project file
// I have failed to make it work automatically with cmake :(
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <angle_windowsstore.h>


#include <vector>
#include <agile.h>


using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::System;
using namespace Platform;


#include <vts-browser/map.hpp>
#include <vts-browser/mapOptions.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/cameraOptions.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-renderer/renderer.hpp>


ref class SingleWindowAppView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
    typedef SingleWindowAppView THISCLASS;

public:
    static THISCLASS ^GetCurrentView(void)
    {
        return CurrentView;
    }

    SingleWindowAppView()
        : isWindowClosed(false), isWindowVisible(false),
        mEglDisplay(EGL_NO_DISPLAY), mEglContext(EGL_NO_CONTEXT), mEglSurface(EGL_NO_SURFACE),
        width(0), height(0)
    {
        CurrentView = this;
    }

    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^applicationView)
    {
        applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &THISCLASS::OnActivated);
    }

    virtual void SetWindow(Windows::UI::Core::CoreWindow ^window)
    {
        hWnd = window;
        window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &THISCLASS::OnVisibilityChanged);
        window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &THISCLASS::OnWindowClosed);
        window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &THISCLASS::OnSizeChanged);
        InitializeOpenGLES();
    }

    virtual void Load(Platform::String ^entryPoint)
    {}

    virtual void Run()
    {
        InitializeVts();
        while (!isWindowClosed)
        {
            if (isWindowVisible)
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
                UpdateVts();
                if (eglSwapBuffers(mEglDisplay, mEglSurface) != GL_TRUE)
                {
                    throw Exception::CreateException(E_FAIL, L"Failed swap buffers");
                }
            }
            else
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }
        FinalizeVts();
        CleanUpOpenGLES();
    }

    virtual void Uninitialize()
    {}

    void OnActivated(CoreApplicationView ^applicationView, IActivatedEventArgs ^args)
    {
        CoreWindow::GetForCurrentThread()->Activate();
    }

    void OnVisibilityChanged(CoreWindow ^sender, VisibilityChangedEventArgs ^args)
    {
        isWindowVisible = args->Visible;
    }

    void OnWindowClosed(CoreWindow ^sender, CoreWindowEventArgs ^args)
    {
        isWindowClosed = true;
    }

    void OnSizeChanged(CoreWindow ^sender, WindowSizeChangedEventArgs ^args)
    {
        // why is this not called when the window is created?
        this->width = args->Size.Width;
        this->height = args->Size.Height;
    }

private:
    bool isWindowClosed, isWindowVisible;
    Platform::Agile<CoreWindow> hWnd;

    static THISCLASS ^CurrentView;

    EGLDisplay mEglDisplay;
    EGLContext mEglContext;
    EGLSurface mEglSurface;

    void InitializeOpenGLES();
    void CleanUpOpenGLES();
    EGLBoolean InitializeEGLDisplayDefault();
    EGLBoolean InitializeEGLDisplay(int level);
    static std::vector<EGLint> MakeDisplayAttribute(int level);

    std::shared_ptr<vts::Map> map;
    std::shared_ptr<vts::renderer::RenderContext> renderContext;
    std::shared_ptr<vts::Camera> camera;
    std::shared_ptr<vts::Navigation> navigation;
    std::shared_ptr<vts::renderer::RenderView> renderView;
    uint32 width, height;

    void InitializeVts();
    void FinalizeVts();
    void UpdateVts();
};

void SingleWindowAppView::InitializeVts()
{
    {
        vts::MapCreateOptions opts;
        opts.clientId = "vts-browser-uwp";
        map = std::make_shared<vts::Map>(opts);
    }
    renderContext = std::make_shared<vts::renderer::RenderContext>();
    renderContext->bindLoadFunctions(&*map);
    camera = map->createCamera();
    navigation = camera->createNavigation();
    renderView = renderContext->createView(&*camera);
    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");
    {
        auto &op = camera->options();
        // rendering geodata crashes the app
        //   given the amount of hacks that had to be done to make it work this far, I don't care anymore
        op.traverseModeGeodata = vts::TraverseMode::None;
        // balanced rendering is suboptimal because Angle does not support GL_EXT_clip_cull_distance
        op.traverseModeSurfaces = vts::TraverseMode::Flat;
    }
}

void SingleWindowAppView::UpdateVts()
{
    // todo input handling
    map->dataUpdate();
    map->renderUpdate(0); // todo elapsed time
    camera->setViewportSize(width, height);
    camera->renderUpdate();
    {
        auto &ro = renderView->options();
        ro.width = width;
        ro.height = height;
    }
    renderView->render();
}

void SingleWindowAppView::FinalizeVts()
{
    map->dataFinalize();
    map->renderFinalize();
    renderView.reset();
    navigation.reset();
    camera.reset();
    renderContext.reset();
    map.reset();
}

SingleWindowAppView ^SingleWindowAppView::CurrentView;

void *glesFunctionLoader(const char *name)
{
    return eglGetProcAddress(name);
}

void SingleWindowAppView::InitializeOpenGLES()
{
    const EGLint configAttributes[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_NONE
    };

    const EGLint contextAttributes[] =
    {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
#ifndef NDEBUG
        EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
#endif
        EGL_NONE
    };

    const EGLint surfaceAttributes[] =
    {
        EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
        EGL_NONE
    };

    if (InitializeEGLDisplayDefault() != EGL_TRUE &&
        InitializeEGLDisplay(0) != EGL_TRUE &&
        InitializeEGLDisplay(1) != EGL_TRUE &&
        InitializeEGLDisplay(2) != EGL_TRUE)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to initialize EGL");
    }

    EGLint numConfigs = 0;
    EGLConfig config = NULL;
    if (eglChooseConfig(mEglDisplay, configAttributes, &config, 1, &numConfigs) != EGL_TRUE)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to enumerate EGLConfigs");
    }
    if (!numConfigs)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to choose first EGLConfig");
    }

    PropertySet ^surfaceCreationProperties = ref new PropertySet();
    surfaceCreationProperties->Insert(ref new String(EGLNativeWindowTypeProperty), hWnd.Get());

    mEglSurface = eglCreateWindowSurface(mEglDisplay, config, reinterpret_cast<IInspectable*>(surfaceCreationProperties), surfaceAttributes);
    if (mEglSurface == EGL_NO_SURFACE)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to create EGL fullscreen surface");
    }

    mEglContext = eglCreateContext(mEglDisplay, config, EGL_NO_CONTEXT, contextAttributes);
    if (mEglContext == EGL_NO_CONTEXT)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to create EGL context");
    }

    if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to make EGLSurface current");
    }

    vts::renderer::loadGlFunctions(&glesFunctionLoader);
}

void SingleWindowAppView::CleanUpOpenGLES()
{
    if (mEglDisplay != EGL_NO_DISPLAY && mEglSurface != EGL_NO_SURFACE)
    {
        eglDestroySurface(mEglDisplay, mEglSurface);
        mEglSurface = EGL_NO_SURFACE;
    }
    if (mEglDisplay != EGL_NO_DISPLAY && mEglContext != EGL_NO_CONTEXT)
    {
        eglDestroyContext(mEglDisplay, mEglContext);
        mEglContext = EGL_NO_CONTEXT;
    }
    if (mEglDisplay != EGL_NO_DISPLAY)
    {
        eglTerminate(mEglDisplay);
        mEglDisplay = EGL_NO_DISPLAY;
    }
}

EGLBoolean SingleWindowAppView::InitializeEGLDisplayDefault()
{
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (EGL_NO_DISPLAY != mEglDisplay)
        return eglInitialize(mEglDisplay, NULL, NULL);
    return EGL_FALSE;
}

EGLBoolean SingleWindowAppView::InitializeEGLDisplay(int level)
{
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (!eglGetPlatformDisplayEXT)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to get function eglGetPlatformDisplayEXT");
    }

    auto attrib = MakeDisplayAttribute(level);

    mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, attrib.data());
    if (mEglDisplay != EGL_NO_DISPLAY)
    {
        return eglInitialize(mEglDisplay, NULL, NULL);
    }
    return EGL_FALSE;
}

std::vector<EGLint> SingleWindowAppView::MakeDisplayAttribute(int level)
{
    const EGLint commonDisplayAttributePre[] =
    {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
    };
    const EGLint commonDisplayAttributePost[] =
    {
        EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
        EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
        EGL_NONE,
    };

    std::vector<EGLint> attrib;
    for (auto v : commonDisplayAttributePre)
    {
        attrib.push_back(v);
    }
    switch (level)
    {
    case 0:
        break;
    case 1:
        attrib.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE);
        attrib.push_back(9);
        attrib.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE);
        attrib.push_back(3);
        break;
    case 2:
        attrib.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
        attrib.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
        break;
    }
    for (auto v : commonDisplayAttributePost)
    {
        attrib.push_back(v);
    }
    return attrib;
}

ref class SingleWindowAppSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView ^CreateView()
    {
        return ref new SingleWindowAppView();
    }
};

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    auto singleWindowAppSource = ref new SingleWindowAppSource();
    CoreApplication::Run(singleWindowAppSource);
    return 0;
}

