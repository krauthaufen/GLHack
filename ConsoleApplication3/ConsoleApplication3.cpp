// ConsoleApplication3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>


#define GL_VERSION_2_0

#include <string.h>
#include <windows.h>
#include <gl/gl.h>
#include "glext.h"
#include "wglext.h"


HWND WINAPI window_from_dc_replacement(HDC dc)
{
	static HWND wnd = NULL;

	if (dc == NULL)
		return NULL;

	if (wnd == NULL) {
		WNDCLASSA wc;
		memset(&wc, 0, sizeof(wc));
		wc.lpfnWndProc = DefWindowProc;
		wc.hInstance = GetModuleHandleA(NULL);
		wc.lpszClassName = "_dummy_window_class_";
		RegisterClassA(&wc);
		wnd = CreateWindowA(wc.lpszClassName, NULL, WS_POPUP, 0, 0, 32, 32, NULL, NULL, wc.hInstance, NULL);
	}

	return wnd;
}

void patch_window_from_dc()
{
	DWORD old_prot;
	unsigned __int64 wfdc = (unsigned __int64)GetProcAddress(GetModuleHandleA("user32.dll"), "WindowFromDC");

	VirtualProtect((void*)wfdc, 14, PAGE_EXECUTE_WRITECOPY, &old_prot);

	// jmp [eip + 0]
	*(char*)(wfdc + 0) = 0xFF;
	*(char*)(wfdc + 1) = 0x25;
	*(unsigned*)(wfdc + 2) = 0x00000000;
	*(unsigned __int64*)(wfdc + 6) = (unsigned __int64)&window_from_dc_replacement;
}


unsigned char buf[4 * 256 * 256];

void test_pbuffer()
{
	PIXELFORMATDESCRIPTOR pfd;
	int pfi;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
	PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
	PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
	PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
	PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;

	int attribs[] = {
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_DRAW_TO_PBUFFER_ARB, 1,
		WGL_COLOR_BITS_ARB, 24,
		0 };
	int pbf;
	uint32_t n;
	HPBUFFERARB pb;
	HDC pbdc;

	char* vendor;
	HGLRC glrc;
	HDC dc = CreateDCA("\\\\.\\DISPLAY4", "\\\\.\\DISPLAY4", NULL, NULL);
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE | PFD_DEPTH_DONTCARE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfi = ChoosePixelFormat(dc, &pfd);
	SetPixelFormat(dc, pfi, &pfd);
	glrc = wglCreateContext(dc);
	wglMakeCurrent(dc, glrc);

	vendor = (char*)glGetString(GL_VENDOR);
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
	wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
	wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
	wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");

	wglChoosePixelFormatARB(dc, attribs, NULL, 1, &pbf, &n);
	pb = wglCreatePbufferARB(dc, pbf, 256, 256, NULL);
	pbdc = wglGetPbufferDCARB(pb);
	wglMakeCurrent(pbdc, glrc);

	glClearColor(0.5f, 0, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glScissor(64, 64, 128, 128);
	glEnable(GL_SCISSOR_TEST);
	glClearColor(0, 1, 0.5f, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buf);
}


void test_fbo(LPCSTR str)
{
	PIXELFORMATDESCRIPTOR pfd;
	int pfi;

	void(_stdcall * glGenRenderbuffers)(GLsizei n, GLuint * renderbuffers);
	void(_stdcall * glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
	void(_stdcall * glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void(_stdcall * glGenFramebuffers)(GLsizei n, GLuint * framebuffers);
	void(_stdcall * glBindFramebuffer)(GLenum target, GLuint framebuffer);
	void(_stdcall * glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	GLenum(_stdcall * glCheckFramebufferStatus)(GLenum target);

	char* vendor;
	GLuint cb, fb;
	GLenum res;

	HGLRC glrc;
	HDC dc = CreateDCA(str, str, NULL, NULL);
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE | PFD_DEPTH_DONTCARE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfi = ChoosePixelFormat(dc, &pfd);
	SetPixelFormat(dc, pfi, &pfd);
	glrc = wglCreateContext(dc);
	wglMakeCurrent(dc, glrc);

	vendor = (char*)glGetString(GL_VENDOR);
	*(PROC*)&glGenRenderbuffers = wglGetProcAddress("glGenRenderbuffers");
	*(PROC*)&glBindRenderbuffer = wglGetProcAddress("glBindRenderbuffer");
	*(PROC*)&glRenderbufferStorage = wglGetProcAddress("glRenderbufferStorage");
	*(PROC*)&glGenFramebuffers = wglGetProcAddress("glGenFramebuffers");
	*(PROC*)&glBindFramebuffer = wglGetProcAddress("glBindFramebuffer");
	*(PROC*)&glFramebufferRenderbuffer = wglGetProcAddress("glFramebufferRenderbuffer");
	*(PROC*)&glCheckFramebufferStatus = wglGetProcAddress("glCheckFramebufferStatus");

	//const char* vendor = (const char*)glGetString(GL_VENDOR);
	printf("%s\n", vendor);

	glGenRenderbuffers(1, &cb);
	glBindRenderbuffer(GL_RENDERBUFFER, cb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, 256, 256);

	glGenFramebuffers(1, &fb);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, cb);

	res = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	//	if (res != GL_FRAMEBUFFER_COMPLETE) _asm int 3

	glClearColor(0.5f, 0, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, buf);
}


int main(int argc, const char* argv[])
{
	patch_window_from_dc();

	char devid[32];

	if (1) {
		DISPLAY_DEVICEA dev;
		int k = 0;
		dev.cb = sizeof(dev);
		dev.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
		while (EnumDisplayDevicesA(NULL, k, &dev, 0))
		{
			if (strstr(dev.DeviceString, "NVIDIA")) {
				strcpy_s(devid, dev.DeviceName);
			}
			printf("%s\n", dev.DeviceString);
			printf("%s\n", dev.DeviceName);
			k += 1;
		}
	}
	//"\\\\.\\DISPLAY1"

	test_fbo(devid);
	//test_pbuffer();

	return 0;
}