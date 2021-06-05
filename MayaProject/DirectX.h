#pragma once
#include "Includes.h"
#include "Nodes.h"
#include "ComLib.h"

using namespace Microsoft::WRL;

class DX
{
private:
	//Core
	ComPtr<ID3D11Device>				gDevice{ nullptr };
	ComPtr<IDXGISwapChain>				gSwapchain{ nullptr };
	ComPtr<ID3D11DeviceContext>			gDeviceContext{ nullptr };
	ComPtr<ID3D11RasterizerState>		gRasterizerState{ nullptr };
	ComPtr<ID3D11DepthStencilView>		gDepthStencilView{ nullptr };
	ComPtr<ID3D11Texture2D>				gDepthStencil[2]{ nullptr, nullptr };
	ComPtr<ID3D11RenderTargetView>		gBackbufferRTV[2]{ nullptr, nullptr };

	ComPtr<ID3D11InputLayout>			gVertexLayout{ nullptr };

	ComPtr<ID3D11VertexShader>			gVertexShader{ nullptr };

	ComPtr<ID3D11GeometryShader>		gGeometryShader{ nullptr };

	ComPtr<ID3D11PixelShader>			gPixelShader{ nullptr };

	ComLib* comlib{ nullptr };
	ComLib* connectionStatus{ nullptr };

	bool currentFrame = { false }; // currentFrame = !currentFrame

	// UP-CAST <-> DOWN-CAST
	std::vector<std::pair<NODETYPES::Node*, std::string&>> pureNodes{};
	std::vector<std::shared_ptr<NODETYPES::Texture>> textures{};
	std::vector<std::shared_ptr<NODETYPES::Bump>> bumps{};
	std::vector<std::shared_ptr<NODETYPES::Lambert>> lamberts{};
	std::vector<std::shared_ptr<NODETYPES::Blinn>> blinns{};

public:



public:
	DX();
	~DX();

	void OfflineCreation(HMODULE hModule, HWND* wndHandle);
	void Update();

private:
	NODETYPES::Node* findNode(std::string uuid)
	{
		for (size_t i = 0; i < this->pureNodes.size(); ++i)
		{
			if (pureNodes[i].second == uuid)
			{
				return pureNodes[i].first;
			}
		}
		return nullptr;
	}

	void addMaterialTextures(char* msg, ComLib::ATTRIBUTE_TYPE attribute);
	void addMaterialChannels(char* msg, ComLib::ATTRIBUTE_TYPE attribute);
	void appendBumpShader(char* msg);
	void updateBump(char* msg);
	void updateTexture(char* msg);

	void addParent(char* msg);
	void allocateNode(char* msg);

	bool loadingObjects();
	void queryExistingData();

	HRESULT CreateDeviceSwapchain(HWND* wndHandle);
	HRESULT CreateDepthBuffer(int index);
	HRESULT CreateDepthStencil(int index, ID3D11Texture2D* pBackBuffer);
	HRESULT CreateRasterizerState();
	void setViewPort();

	void CreateShaders();
	HRESULT CreateInputLayout(D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT descSize, ID3DBlob* VS);
	HRESULT CreateVertexShader(LPCWSTR fileName, LPCSTR entryPoint, D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT arraySize);
	HRESULT CreateGeometryShader(LPCWSTR fileName, LPCSTR entryPoint);
	HRESULT CreatePixelShader(LPCWSTR fileName, LPCSTR entryPoint);

	void createConstantBuffers();

	void CreateDirect3DContext(HWND* wndHandle);
};