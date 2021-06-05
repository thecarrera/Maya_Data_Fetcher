#pragma once

#pragma comment(lib, "windowscodecs.lib")

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

#include <wincodec.h>
#include <wrl.h>
#include "WICTextureLoader.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")
#pragma comment (lib, "dxguid.lib")

#define WLABEL "3D Projekt Jonathan Carrera"
#define gICON  "Icon.ico"
#define HEIGHT 1080.0
#define WIDTH 1920.0
#define PIXELSAMPLE 1

#define SAFE_RELEASE(x) if(x) x->Release(), x = nullptr
#define SAFE_DELETE(x) if(x) delete[] x, x = nullptr

//#define SAFE_LOOP2_DELETE(x,y) if(x) for(int i = 0; i < y; i++){if(x[i]) delete x[i]; } delete[] x, x = nullptr

struct camMatrices
{
	DirectX::XMMATRIX viewM = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX projM = DirectX::XMMatrixIdentity();
};

struct objMatrices
{
	DirectX::XMMATRIX worldM = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX invWorldM = DirectX::XMMatrixIdentity();
};

struct lightMatrices
{
	DirectX::XMFLOAT4 lightPos;
};