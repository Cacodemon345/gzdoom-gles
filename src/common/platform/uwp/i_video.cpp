#ifdef ZMUSIC_STATIC
#define KHRONOS_STATIC
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <Windows.h>
#include <windows.foundation.h>
#include <windows.ui.core.h>
#include <windows.storage.h>
#include <windows.applicationmodel.h>
#include <windows.ui.viewmanagement.h>
#include <wrl.h>
#include "i_main.h"
#include "m_argv.h"
#include "cmdlib.h"
#include "version.h"
#include "d_eventbase.h"
#include "i_sound.h"
#include "c_cvars.h"
#include "engineerrors.h"
#include "gl_sysfb.h"
#include "i_video.h"
#include "hardware.h"
#include "gl_system.h"

#include "gl_renderer.h"
#include "gl_framebuffer.h"
#include "gles_framebuffer.h"
#ifdef HAVE_SOFTPOLY
#include "poly_framebuffer.h"
#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#endif

#include <vector>
#include <ppltasks.h>
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

static CoreWindow^ gameWindow;
#ifdef HAVE_SOFTPOLY
static bool winsizechanged = false; // Needed for Softpoly;
#endif
IVideo* Video = nullptr;
bool AppInit = false;
extern bool AppActive;

EXTERN_CVAR (Int, vid_preferbackend);
CVAR (Bool, i_soundinbackground, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG);

class UWPVideo : public IVideo
{
public:
	DFrameBuffer* CreateFrameBuffer();
	~UWPVideo();
	UWPVideo();
};

UWPVideo::UWPVideo()
{
	// Real magic happens in the framebuffer creation function.
	AppInit = true;
}

UWPVideo::~UWPVideo() {}

void I_SetIWADInfo() {}

DFrameBuffer* UWPVideo::CreateFrameBuffer()
{
	// Vulkan isn't going to be supported until gfx-rs/portability finally gets UWP support for real. Desktop OpenGL is out of question.
	bool softpoly = vid_preferbackend == 2;
#ifdef HAVE_SOFTPOLY
	if (softpoly)
	{
		return new PolyFrameBuffer(0, vid_fullscreen);
	}
	else
#endif
	{
		return new OpenGLESRenderer::OpenGLFrameBuffer(0, vid_fullscreen);
	}
}

SystemBaseFrameBuffer::SystemBaseFrameBuffer(void* hMonitor, bool fullscreen)
{
	if (fullscreen)
	{
		Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->TryEnterFullScreenMode();
	}
}

bool SystemBaseFrameBuffer::IsFullscreen()
{
	return Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->IsFullScreenMode;
}

void SystemBaseFrameBuffer::ToggleFullscreen(bool yes)
{
	auto view = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
	if (!yes)
	{
		view->ExitFullScreenMode();
	}
	else
	{
		view->TryEnterFullScreenMode();
	}
	if (!yes)
	{
		if (!fullscreenSwitch)
		{
			fullscreenSwitch = true;
			vid_fullscreen = false;
		}
		else
		{
			fullscreenSwitch = false;
			SetWindowSize(win_w, win_h);
		}
	}
	auto rect = view->VisibleBounds;
	win_x = rect.Left;
	win_y = rect.Top;
	win_w = rect.Width;
	win_h = rect.Height;
}

bool I_GetVulkanPlatformExtensions(unsigned int*, char const**) { return false; }
void I_GetVulkanDrawableSize(int* width, int* height)
{
	*width = (int)Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->VisibleBounds.Width;
	*height = (int)Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->VisibleBounds.Height;
}
bool I_CreateVulkanSurface(struct VkInstance_T*, struct VkSurfaceKHR_T** surf) { *surf = nullptr; return false; }

FString I_GetFromClipboard(bool uses_primary_selection)
{
	auto clipView = Windows::ApplicationModel::DataTransfer::Clipboard::GetContent();
	if (clipView && clipView->Contains(Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text))
	{
		std::atomic<bool> taskcomplete = false;
		FString cliptext;
		auto task =
			Concurrency::create_task(clipView->GetTextAsync()).then([&](Platform::String^ str)
			{
				cliptext = str->Data(); taskcomplete = true;
				return cliptext;
			});
		while (!taskcomplete) { Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessOneIfPresent); }
		return cliptext;
	}
	return "";
}

void I_PutInClipboard(const char* data)
{
	DataPackage^ pkg = ref new DataPackage;
	pkg->RequestedOperation = DataPackageOperation::Copy;
	pkg->SetData(Windows::ApplicationModel::DataTransfer::StandardDataFormats::Text, ref new Platform::String(FString(data).WideString().c_str()));
	Clipboard::SetContent(pkg);
}

void I_SetWindowTitle(const char* title)
{
	if (title)
	{
		Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->Title = ref new Platform::String(FString(title).WideString().c_str());
		return;
	}
	FString default_caption;
	default_caption.Format(GAMENAME " %s (%s)", GetVersionString(), GetGitTime());
	Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->Title = ref new Platform::String(default_caption.WideString().c_str());
}

int SystemBaseFrameBuffer::GetClientWidth()
{
	auto view = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
	return view->VisibleBounds.Width;
}

int SystemBaseFrameBuffer::GetClientHeight()
{
	auto view = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
	return view->VisibleBounds.Height;
}

void SystemBaseFrameBuffer::SetWindowSize(int width, int height)
{
	if (width < VID_MIN_WIDTH || height < VID_MIN_HEIGHT)
	{
		width = VID_MIN_WIDTH;
		height = VID_MIN_HEIGHT;
	}
	win_w = width;
	win_h = height;
	if (vid_fullscreen)
	{
		vid_fullscreen = false;
	}
	else
	{
		Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->TryResizeView(Windows::Foundation::Size(width, height));
		auto rect = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->VisibleBounds;
		win_x = rect.Left;
		win_y = rect.Top;
		win_w = rect.Width;
		win_h = rect.Top;
	}
}

const EGLAttrib defaultDisplayAttributes[] =
{
	EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
	EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
	EGL_NONE,
};

SystemGLFrameBuffer::SystemGLFrameBuffer(void* hMonitor, bool fullscreen)
	: SystemBaseFrameBuffer(hMonitor, fullscreen)
{
	if (!GLDisplay)
	{
		GLDisplay = eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
		if (!GLDisplay)
		{
			I_FatalError("Could not get ANGLE EGL display.");
		}
		if (eglInitialize(GLDisplay, NULL, NULL) != EGL_TRUE)
		{
			GLDisplay = 0;
			I_FatalError("Could not initialize EGL.");
		}
	}
	const EGLint configAttribs[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	const EGLint surfaceAttribs[1] = { EGL_NONE };
	Windows::Foundation::Collections::PropertySet^ propset = ref new Windows::Foundation::Collections::PropertySet();
	propset->Insert(ref new Platform::String(L"EGLNativeWindowTypeProperty"), gameWindow);
	EGLint numConfigs = 0;
	EGLConfig config = 0;
	if ((eglChooseConfig(GLDisplay, configAttribs, &config, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
	{
		I_FatalError("Matching EGLConfig not found.");
	}
	if ((GLSurface = eglCreateWindowSurface(GLDisplay, config, reinterpret_cast<IInspectable*>(propset), surfaceAttribs)) == EGL_NO_SURFACE)
	{
		I_FatalError("Could not create EGL surface.");
	}
	const EGLint contextAttributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	GLContext = eglCreateContext(GLDisplay, config, EGL_NO_CONTEXT, contextAttributes);
	if (GLContext == EGL_NO_CONTEXT)
	{
		I_FatalError("Failed to create context.");
	}
	if (eglMakeCurrent(GLDisplay, GLSurface, GLSurface, GLContext) == EGL_FALSE)
	{
		I_FatalError("Failed to make EGL context current.");
	}
}

void SystemGLFrameBuffer::SwapBuffers()
{
	eglSwapBuffers(GLDisplay, GLSurface);
}

void SystemGLFrameBuffer::SetVSync(bool yes)
{
	eglSwapInterval(GLDisplay, (EGLint)(yes));
}

int SystemGLFrameBuffer::GetClientWidth()
{
	return Super::GetClientWidth();
}

int SystemGLFrameBuffer::GetClientHeight()
{
	return Super::GetClientHeight();
}

SystemGLFrameBuffer::~SystemGLFrameBuffer()
{
	eglDestroyContext(GLDisplay, GLContext);
	eglDestroySurface(GLDisplay, GLSurface);
	eglTerminate(GLDisplay);
}

IVideo* gl_CreateVideo()
{
	return new UWPVideo();
}

void I_InitGraphics()
{
	Video = gl_CreateVideo();
	if (!Video)
	{
		I_FatalError("Failed to initialize display.");
	}
}

void I_ShutdownGraphics()
{
	delete Video;
}

void GameApp::SetWindow(CoreWindow^ window)
{
	window->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &GameApp::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &GameApp::OnVisibilityChanged);

	window->Closed +=
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &GameApp::OnWindowClosed);
	
	Windows::Devices::Input::MouseDevice::GetForCurrentView()->MouseMoved +=
		ref new TypedEventHandler<Windows::Devices::Input::MouseDevice^, Windows::Devices::Input::MouseEventArgs^>(this, &GameApp::OnMouseMoved);

	window->KeyDown +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &GameApp::OnKeyDown);

	window->KeyUp +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &GameApp::OnKeyUp);

	window->CharacterReceived +=
		ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &GameApp::OnCharacterReceived);

	window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &GameApp::OnPointerPressed);

	window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &GameApp::OnPointerReleased);

	window->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &GameApp::OnPointerMoved);

	window->PointerWheelChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::PointerEventArgs^>(this, &GameApp::OnPointerWheelChanged);

	Windows::UI::Core::Preview::SystemNavigationManagerPreview::GetForCurrentView()->CloseRequested +=
		ref new EventHandler<Windows::UI::Core::Preview::SystemNavigationCloseRequestedPreviewEventArgs^>(this, &GameApp::OnCloseRequested);

	gameWindow = window;
}

void GameApp::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	if (AppInit)
	{
		win_w = args->Size.Width;
		win_h = args->Size.Height;
#ifdef HAVE_SOFTPOLY
		winsizechanged = true;
#endif
	}
}

void GameApp::OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args)
{
	if (!AppInit)
	{
		S_SetSoundPaused(i_soundinbackground);
		AppActive = false;
	}
}

void GameApp::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	if (!AppInit)
	{
		S_SetSoundPaused(1);
		AppActive = true;
	}
}

#ifdef HAVE_SOFTPOLY
namespace
{
	int SrcWidth = 0;
	int SrcHeight = 0;
	int ClientWidth = 0;
	int ClientHeight = 0;
	bool CurrentVSync = false;

	IDXGIAdapter1* dxgiAdapter = nullptr;
	IDXGIDevice2* dxgiDevice = nullptr;
	ID3D11Device* d3dDevice = nullptr;
	ID3D11DeviceContext* d3dContext = nullptr;
	IDXGIFactory2* dxgiFactory = nullptr;
	IDXGISwapChain1* dxgiSwapChain = nullptr;
	ID2D1Factory1* d2dFactory = nullptr;
	ID2D1Device* d2dDevice = nullptr;
	ID2D1DeviceContext* d2dContext = nullptr;
	ID2D1Bitmap1* d2dBitmap = nullptr;
	ID2D1Bitmap1* d2dInterBitmap = nullptr;
	IDXGISurface* dxgiSurface = nullptr;
	uint8_t* interData = nullptr;
}

#define THROWIFFAILED(x) { HRESULT hr = x; if (FAILED(hr)) { I_FatalError("%s failed: %s failed: 0x%X\n", __func__, #x, hr); } }

void I_PolyPresentInit()
{
	THROWIFFAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&dxgiFactory));
	THROWIFFAILED(dxgiFactory->EnumAdapters1(0, &dxgiAdapter));
	const D3D_FEATURE_LEVEL levels[] = 
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3
	};
	THROWIFFAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
									nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, sizeof(levels) / sizeof(levels[0]), D3D11_SDK_VERSION, &d3dDevice, nullptr, &d3dContext))
	THROWIFFAILED(d3dDevice->QueryInterface(&dxgiDevice));
	D2D1_FACTORY_OPTIONS factoryOptions = { D2D1_DEBUG_LEVEL_NONE };
	THROWIFFAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), &factoryOptions, (void**)&d2dFactory));
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.Flags = 0;
	THROWIFFAILED(dxgiFactory->CreateSwapChainForCoreWindow(d3dDevice, reinterpret_cast<IUnknown*>(gameWindow), &swapChainDesc, nullptr, &dxgiSwapChain));
	THROWIFFAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory))
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1( D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
																	   D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0,0);
	THROWIFFAILED(dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface)));
	THROWIFFAILED(d2dFactory->CreateDevice(dxgiDevice, &d2dDevice))
	THROWIFFAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dContext))
	THROWIFFAILED(d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface, bitmapProperties, &d2dBitmap));
	d2dContext->SetTarget(d2dBitmap);
	interData = new uint8_t[2048 * 2048 * 4];
	
}

void I_PolyPresentDeinit()
{
	if (d2dBitmap) d2dBitmap->Release();
	if (d2dContext) d2dContext->Release();
	if (d2dDevice) d2dDevice->Release();
	if (d2dFactory) d2dFactory->Release();
	if (dxgiSurface) dxgiSurface->Release();
	if (dxgiSwapChain) dxgiSwapChain->Release();
	if (d3dDevice) d3dDevice->Release();
	if (dxgiAdapter) dxgiAdapter->Release();
	if (dxgiFactory) dxgiFactory->Release();
}

uint8_t* I_PolyPresentLock(int w, int h, bool vsync, int& pitch)
{
	CurrentVSync = vsync;
	if (dxgiSurface)
	{
		d2dContext->SetTarget(nullptr);
		d3dContext->ClearState();
		d2dBitmap->Release();
		d2dBitmap = nullptr;
		d2dDevice->Release();
		d2dDevice = nullptr;
		dxgiSurface->Release();
		dxgiSurface = nullptr;

		if (winsizechanged)
		{
			d3dContext->Flush();
			THROWIFFAILED(dxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)); winsizechanged = false;
		};
		THROWIFFAILED(dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface)));
		THROWIFFAILED(d2dFactory->CreateDevice(dxgiDevice, &d2dDevice));
		D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
																		   D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0);
		THROWIFFAILED(d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface, bitmapProperties, &d2dBitmap));
		d2dContext->SetTarget(d2dBitmap);
	}
	THROWIFFAILED(d2dContext->CreateBitmap(D2D1::SizeU(w, h), nullptr, w * 4, D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
							D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0), &d2dInterBitmap));
	
	pitch = 2048 * 4;
	return interData;
}

void I_PolyPresentUnlock(int x, int y, int w, int h)
{
	D2D1_RECT_U dstRect = { 0, 0, d2dInterBitmap->GetSize().width, d2dInterBitmap->GetSize().height};
	d2dInterBitmap->CopyFromMemory(&dstRect, interData, 2048 * 4);
	d2dContext->BeginDraw();
	d2dContext->Clear(D2D1::ColorF(0.5,0.5,0.5,0.5));
	D2D1_RECT_F srcrect = D2D1::RectF(x, y, w, h);
	d2dContext->DrawBitmap(d2dInterBitmap, nullptr, 1.0f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &srcrect);
	THROWIFFAILED(d2dContext->EndDraw());
	THROWIFFAILED(dxgiSwapChain->Present((unsigned int)CurrentVSync, 0));
	d2dInterBitmap->Release();
	d2dInterBitmap = nullptr;
}
#endif