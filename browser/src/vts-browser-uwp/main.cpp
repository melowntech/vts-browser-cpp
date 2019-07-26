
// code based on http://ysflight.in.coocan.jp/programming/uwp_gles2_cmake/e.html

#include <vector>
#include <agile.h>

#if !defined GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <GLES3/gl3.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <angle_windowsstore.h>


using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
//using namespace Windows::UI::Input;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
//using namespace Windows::Graphics::Display;
using namespace Windows::System;
using namespace Platform;


#include <vts-browser/map.hpp>
#include <vts-browser/camera.hpp>
#include <vts-browser/navigation.hpp>
#include <vts-browser/mapOptions.hpp>
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
        mEglDisplay(EGL_NO_DISPLAY), mEglContext(EGL_NO_CONTEXT), mEglSurface(EGL_NO_SURFACE)
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

    void InitializeVts();
    void FinalizeVts();
    void UpdateVts();
};

void SingleWindowAppView::InitializeVts()
{
    {
        vts::MapCreateOptions opts;
        opts.diskCache = false;
        map = std::make_shared<vts::Map>(opts);
    }
    renderContext = std::make_shared<vts::renderer::RenderContext>();
    renderContext->bindLoadFunctions(&*map);
    camera = map->createCamera();
    navigation = camera->createNavigation();
    renderView = renderContext->createView(&*camera);
    map->dataInitialize();
    map->renderInitialize();
    map->setMapconfigPath("https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json");
}

void SingleWindowAppView::UpdateVts()
{
    map->dataUpdate();
    map->renderUpdate(0);

    glClearColor(0, 0, 1, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    case 1:  // FL9.  I don't know exactly what FL9 stands for, but as example says.
        attrib.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE);
        attrib.push_back(9);
        attrib.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE);
        attrib.push_back(3);
        break;
    case 2:  // warp.  I don't know exactly what it means by "warp", but as example says.
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

