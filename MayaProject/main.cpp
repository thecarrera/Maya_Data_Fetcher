#include "DirectX.h"

HWND InitWindow(HMODULE hModule);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI wWinMain(HMODULE hModule, HMODULE hPrevModule, LPWSTR lpCmdLine, int nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stdout);

	DX dx;
	MSG msg = { 0 };
	HWND wndHandle = InitWindow(hModule);

	if (wndHandle)
	{
		ShowWindow(wndHandle, nCmdShow);
		dx.OfflineCreation(hModule, &wndHandle);


		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				dx.Update();
			}
		}
		DestroyWindow(wndHandle);
	}

	//Send Message to Plugin about termination.

	dx.disconnect();

	FreeConsole();

	return (int)msg.wParam;
}

HWND InitWindow(HMODULE hModule)
{
	HICON icon = (HICON)LoadImage(NULL,
		"Icon.ico",
		IMAGE_ICON,
		0,
		0,
		LR_LOADFROMFILE |
		LR_DEFAULTSIZE |
		LR_SHARED);


	WNDCLASSEX wce = { 0 };
	wce.hIcon = icon;
	wce.cbSize = sizeof(WNDCLASSEX);
	wce.style = CS_HREDRAW | CS_VREDRAW;
	wce.lpfnWndProc = WndProc;
	wce.hInstance = hModule;
	wce.lpszClassName = WLABEL;
	if (!RegisterClassEx(&wce))
	{
		exit(-1);
	}

	RECT rc = { 0, 0, (long)WIDTH, (long)HEIGHT };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

	auto wWidth = rc.right - rc.left;
	auto wHeight = rc.bottom - rc.top;

	HWND handle = CreateWindow(
		WLABEL,
		WLABEL,
		WS_OVERLAPPEDWINDOW,
		((GetSystemMetrics(SM_CXSCREEN) / 2) - (wWidth / 2)),
		((GetSystemMetrics(SM_CYSCREEN) / 2) - (wHeight / 2)),
		wWidth,
		wHeight,
		nullptr,
		nullptr,
		hModule,
		nullptr);

	return handle;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:

		if (wp == VK_ESCAPE)
		{

			PostQuitMessage(0);
			break;

		}
	}
	return DefWindowProc(hWnd, msg, wp, lp);
}
