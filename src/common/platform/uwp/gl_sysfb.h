#pragma once
#include <EGL/egl.h>
#include "v_video.h"
#include "i_main.h"

class SystemBaseFrameBuffer : public DFrameBuffer
{
	typedef DFrameBuffer Super;

	public:
	// this must have the same parameters as the Windows version, even if they are not used!
	SystemBaseFrameBuffer(void* hMonitor, bool fullscreen);

	bool IsFullscreen() override;

	int GetClientWidth() override;
	int GetClientHeight() override;

	void ToggleFullscreen(bool yes) override;
	void SetWindowSize(int client_w, int client_h) override;

	protected:
	SystemBaseFrameBuffer() {}
	GameApp^ app;
	bool fullscreenSwitch;
};

class SystemGLFrameBuffer : public SystemBaseFrameBuffer
{
	typedef SystemBaseFrameBuffer Super;

	public:
	SystemGLFrameBuffer(void* hMonitor, bool fullscreen);
	~SystemGLFrameBuffer();

	int GetClientWidth() override;
	int GetClientHeight() override;

	virtual void SetVSync(bool vsync) override;
	void SwapBuffers();

	protected:
	EGLContext GLContext;
	EGLSurface GLSurface;
	EGLDisplay GLDisplay;

	SystemGLFrameBuffer() {}
};