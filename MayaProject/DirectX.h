#pragma once
#include "Includes.h"
#include "Nodes.hpp"
#include "ComLib.h"

using namespace Microsoft::WRL;

class DX
{
private:
	//Core
	ComPtr<ID3D11Device>				gDevice						{};
	ComPtr<ID3D11DeviceContext>			gDeviceContext				{};
	ComPtr<IDXGISwapChain>				gSwapchain					{};
	ComPtr<ID3D11RasterizerState>		gRasterizerState			{};
	ComPtr<ID3D11Texture2D>				gDepthStencil[2]			{};
	ComPtr<ID3D11DepthStencilView>		gDepthStencilView[2]		{};
	ComPtr<ID3D11RenderTargetView>		gBackbufferRTV[2]			{};

	//Default Textures
	ComPtr<ID3D11Texture2D>				defaultTextureMap			{};
	ComPtr<ID3D11ShaderResourceView>	defaultTextureSRV			{};
	ComPtr<ID3D11ShaderResourceView>	nullSRV						{};

	//G-Buffer
	ComPtr<ID3D11Texture2D>				gGBufferTextures[3]			{};
	ComPtr<ID3D11RenderTargetView>		gGBufferRTVs[3]				{};
	ComPtr<ID3D11ShaderResourceView>	gGBufferSRVs[3]			{};


	struct ACTIVEPOINTLIGHTS{
		UINT32 pointLightCount {};
		NODETYPES::PointLight::POINTDATA pointLight[20] {};
	}activePointLights{};

	//Shaders
	ComPtr<ID3D11InputLayout>			gVertexLayout				{};
	ComPtr<ID3D11VertexShader>			gGBufferVertexShader		{};
	ComPtr<ID3D11GeometryShader>		gGBufferGeometryShader		{};
	ComPtr<ID3D11PixelShader>			gGBufferPixelShader			{};

	ComPtr<ID3D11PixelShader>			gLightPassShader			{};

	ComPtr<ID3D11SamplerState>			txSamplerState				{};
	
	ComLib*								comlib						{};

	bool currentFrame {}; // currentFrame = !currentFrame

	// UP-CAST <-> DOWN-CAST
	std::vector<std::pair<NODETYPES::Node*, std::string>>		pureNodes				{};

	std::vector<std::shared_ptr<NODETYPES::Camera>>				cameras					{};
	NODETYPES::Camera*											activeCamera			{};
	ComPtr<ID3D11Buffer>										cameraBuffer[2]			{}; // 128-bit - MAT(16-bit Aligned)

	std::vector<std::shared_ptr<NODETYPES::Mesh>>				meshes					{};
	std::vector<std::shared_ptr<NODETYPES::Transform>>			transforms				{};
	ComPtr<ID3D11Buffer>										transformBuffer			{}; // 64-bit - XMMATRIX(16-bit Aligned)

	std::vector<std::shared_ptr<NODETYPES::PointLight>>			pointLights				{};
	ComPtr<ID3D11Buffer>										pointLightDataBuffer	{};  

	std::vector<std::shared_ptr<NODETYPES::Texture>>			textures				{};
	std::vector<std::shared_ptr<NODETYPES::Bump>>				bumps					{};
	std::vector<std::shared_ptr<NODETYPES::Lambert>>			lamberts				{};
	std::vector<std::shared_ptr<NODETYPES::Blinn>>				blinns					{};
	std::vector<std::shared_ptr<NODETYPES::ShadingEngine>>		shadingEngines			{};


public:



public:
	DX();
	~DX();
	void deallocateNodes();
	void disconnect();


	void OfflineCreation(HMODULE hModule, HWND* wndHandle);
	void Update();
	void Render();
	
	void ClearGBufferTargets();
	void RenderGBuffer();
	void UpdateTransformBuffer(size_t index);
	void UpdateCameraBuffers();
	void updateGBufferShaders(size_t index);
	void clearRender();
	void RenderLightPass();
	void UpdatePointLightBuffers();
	void updateVertexBuffers(UINT meshIndex, UINT faceIndex);

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

	void addShaderEngineMaterials(char* msg);
	void addMaterialTextures(char* msg, ComLib::ATTRIBUTE_TYPE attribute);
	void addMaterialChannels(char* msg, ComLib::ATTRIBUTE_TYPE attribute);
	void appendBumpShader(char* msg);

	void updatePointLight(char* msg);

	void updateMatrix(char* msg, ComLib::ATTRIBUTE_TYPE attribute);
	void updateList(char* msg, ComLib::ATTRIBUTE_TYPE attribute);
	void updateMeshTransform(char* msg);
	void updateMeshShaders(char* msg);
	void updateBump(char* msg);
	void updateTexture(char* msg);

	void addParent(char* msg);
	void removeParent(char* msg);
	void setActiveCamera(char* msg);
	void allocateNode(char* msg);
	void deallocateNode(char* msg);
	void allocateVertex(char* msg);
	void allocateFaces(char* msg);

	HRESULT loadingObjects();
	HRESULT queryExistingData();

	void createDefaultTextures();
	void CreateSamplerStates();
	void CreateBuffers();
	HRESULT CreateDeviceSwapchain(HWND* wndHandle);
	HRESULT CreateDepthBuffer(int index);
	HRESULT CreateDepthStencil(int index, ID3D11Texture2D* pBackBuffer);
	HRESULT CreateGBufferResources();
	HRESULT CreateRasterizerState();
	void setViewPort();

	void CreateShaders();
	HRESULT CreateInputLayout(D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT descSize, ID3DBlob* VS);
	HRESULT CreateVertexShader(LPCWSTR fileName, LPCSTR entryPoint, D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT arraySize, ID3D11VertexShader*& vertexShader);
	HRESULT CreateGeometryShader(LPCWSTR fileName, LPCSTR entryPoint, ID3D11GeometryShader*& geometricShader);
	HRESULT CreatePixelShader(LPCWSTR fileName, LPCSTR entryPoint, ID3D11PixelShader*& pixelShader);

	void createConstantBuffers();

	void CreateDirect3DContext(HWND* wndHandle);
};