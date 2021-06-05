#include "DirectX.h"

DX::DX()
{
	this->comlib = new ComLib("sharedFileMap", (1ULL << ((64ULL * 25000000ULL) - 1ULL)));
	this->connectionStatus = new ComLib("connection", 1ULL << ((64ULL * 10ULL) - 1ULL));
}
DX::~DX()
{
	delete comlib;
	delete connectionStatus;
}

void DX::addMaterialTextures(char* msg, ComLib::ATTRIBUTE_TYPE attribute)
{
	size_t messageOffset{};

	size_t uuidSize{};
	std::string uuid{};
	bool textureConnected{};
	size_t connectedTextureCount{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* materialNode{ this->findNode(uuid) };
	if (materialNode)
	{
		memcpy(&textureConnected, msg + messageOffset, sizeof(bool));
		messageOffset += sizeof(bool);

		if (textureConnected)
		{
			if (materialNode->getType() == "lambert")
			{
				NODETYPES::Lambert* lambert = dynamic_cast<NODETYPES::Lambert*>(materialNode);
				std::vector<NODETYPES::Node*> textureUuids;
				
				lambert->setTextureConnected(textureConnected, attribute);

				memcpy(&connectedTextureCount, msg + messageOffset, STSIZE);
				messageOffset += STSIZE;

				size_t textureUuidSize{};
				std::string textureUuid{};
				for (size_t i = 0; i < connectedTextureCount; ++i)
				{
					memcpy(&textureUuidSize, msg + messageOffset, STSIZE);
					messageOffset += STSIZE;
					memcpy(&textureUuid[0], msg + messageOffset, textureUuidSize);
					messageOffset += textureUuidSize;

					NODETYPES::Node* textureNode{ this->findNode(textureUuid) };
					textureUuids.emplace_back(textureNode);
				}
				lambert->setTextureMaps(textureUuids, attribute);
			}
			else if (materialNode->getType() == "blinn")
			{
				NODETYPES::Blinn* blinn = dynamic_cast<NODETYPES::Blinn*>(materialNode);
				std::vector<NODETYPES::Node*> textureUuids;

				blinn->setTextureConnected(textureConnected, attribute);

				memcpy(&connectedTextureCount, msg + messageOffset, STSIZE);
				messageOffset += STSIZE;

				size_t textureUuidSize{};
				std::string textureUuid{};
				for (size_t i = 0; i < connectedTextureCount; ++i)
				{
					memcpy(&textureUuidSize, msg + messageOffset, STSIZE);
					messageOffset += STSIZE;
					memcpy(&textureUuid[0], msg + messageOffset, textureUuidSize);
					messageOffset += textureUuidSize;

					NODETYPES::Node* textureNode{ this->findNode(textureUuid) };
					textureUuids.emplace_back(textureNode);
				}
				blinn->setTextureMaps(textureUuids, attribute);
			}
		}
	}
}
void DX::addMaterialChannels(char* msg, ComLib::ATTRIBUTE_TYPE attribute)
{
	size_t messageOffset {};

	size_t uuidSize{};
	std::string uuid{};
	float container[3]{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;
	memcpy(&container, msg + messageOffset, sizeof(float[3]));
	messageOffset += sizeof(float[3]);

	NODETYPES::Node* node{ this->findNode(uuid) };
	if (node)
	{
		if(node->getType() == "lambert")
		{
			NODETYPES::Lambert* lambert = dynamic_cast<NODETYPES::Lambert*>(node);
			if (lambert)
			{
				lambert->setChannels(container, attribute);
			}
		}
		else if (node->getType() == "blinn")
		{
			NODETYPES::Blinn* blinn = dynamic_cast<NODETYPES::Blinn*>(node);
			if (blinn)
			{
				blinn->setChannels(container, attribute);
			}
		}
	}
}
void DX::appendBumpShader(char* msg)
{
	size_t		messageOffset	{};

	std::string	uuid			{};
	size_t		uuidSize		{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node{ this->findNode(uuid) };
	if (node)
	{
		NODETYPES::Bump* bump {dynamic_cast<NODETYPES::Bump*>(node)};
		if (bump)
		{
			std::string shaderUuid		{};
			size_t		shaderUuidSize	{};
			memcpy(&shaderUuidSize, msg + messageOffset, STSIZE);
			messageOffset += STSIZE;
			memcpy(&shaderUuid[0], msg + messageOffset, shaderUuidSize);
			messageOffset += shaderUuidSize;	

			NODETYPES::Node* shaderNode{this->findNode(shaderUuid)};
			if (shaderNode)
			{
				bump->addShader(shaderNode);
			}
		}
	}
}
void DX::updateBump(char* msg)
{
	size_t		messageOffset	{};

	std::string uuid			{};
	size_t		uuidSize		{};
	size_t		textureCount	{};
	
	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node { this->findNode(uuid) };
	if (node)
	{
		memcpy(&textureCount, msg + messageOffset, STSIZE);
		messageOffset += STSIZE;
		if (textureCount)
		{
			std::vector<NODETYPES::Node*> textureUuids{};
			for (size_t i = 0; i < textureCount; ++i)
			{
				size_t textureUuidSize{};
				std::string textureUuid{};

				memcpy(&textureUuidSize, msg + messageOffset, STSIZE);
				messageOffset += STSIZE;
				memcpy(&textureUuid[0], msg + messageOffset, textureUuidSize);
				messageOffset += textureUuidSize;

				textureUuids.emplace_back(this->findNode(textureUuid));
			}

			NODETYPES::Bump* bump = dynamic_cast<NODETYPES::Bump*>(node);
			if (bump)
			{
				bump->setTextures(textureUuids);
			}
		}
	}
}
void DX::updateTexture(char* msg)
{
	size_t		messageOffset	{};

	std::string uuid			{};
	size_t		uuidSize		{};
	bool		existingTexture	{};
	std::string	path			{};
	size_t		pathSize		{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;
	
	NODETYPES::Node* node { this->findNode(uuid) };
	if (node)
	{
		memcpy(&existingTexture, msg + messageOffset, sizeof(bool));
		messageOffset += sizeof(bool);

		if (existingTexture)
		{
			memcpy(&pathSize, msg + messageOffset, STSIZE);
			messageOffset += STSIZE;
			memcpy(&path[0], msg + messageOffset, pathSize);
			messageOffset += pathSize;

				NODETYPES::Texture* texture = dynamic_cast<NODETYPES::Texture*>(node);
				if (texture)
				{
					texture->toggleExistTexture(existingTexture);
					texture->setFilePath(path);
				}
		}
	}
}

void DX::addParent(char* msg)
{
	size_t messageOffset{ 0 };
	size_t targetUuidSize{};
	std::string targetUuid{};
	memcpy(&targetUuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	memcpy(&targetUuid[0], msg + messageOffset, targetUuidSize);
	messageOffset += targetUuidSize;

	for (size_t i = 0; i < this->pureNodes.size(); ++i)
	{
		if (pureNodes[i].second == targetUuid)
		{
			NODETYPES::Node* node{ pureNodes[i].first };

		}
	}
}
void DX::allocateNode(char* msg)
{
	size_t messageOffset{0};
	
	size_t typeSize{};
	std::string type{};
	size_t nameSize{};
	std::string name{};
	size_t uuidSize{};
	std::string uuid{};

	memcpy(&typeSize, msg, STSIZE);
	messageOffset += STSIZE;
	memcpy(&type[0], msg + messageOffset, typeSize);
	messageOffset += typeSize;

	memcpy(&nameSize, msg + messageOffset, STSIZE);
	messageOffset += STSIZE;
	memcpy(&name[0], msg + messageOffset, nameSize);
	messageOffset += nameSize;
	
	memcpy(&uuidSize, msg + messageOffset, STSIZE);
	messageOffset += STSIZE;
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	if (type.c_str() == "transform") // NOTE: Change the rest
	{
		
	}
	else if (std::strcmp(type.c_str(), "mesh"))
	{

	}
	else if (strcmp(type.c_str(), "pointLight"))
	{

	}
	else if (strcmp(type.c_str(), "camera"))
	{

	}
	else if (strcmp(type.c_str(), "blinn"))
	{
		NODETYPES::Blinn* blinn{ new NODETYPES::Blinn(name, uuid, "blinn")};
		this->pureNodes.emplace_back(blinn, uuid);
		this->blinns.emplace_back(blinn);
		
	}
	else if (strcmp(type.c_str(), "lambert"))
	{
		NODETYPES::Lambert* lambert{ new NODETYPES::Lambert(name, uuid, "lambert") };
		this->pureNodes.emplace_back(lambert, uuid);
		this->lamberts.emplace_back(lambert);
	}
	else if (std::strcmp(type.c_str(), "file"))
	{
		NODETYPES::Texture* tex{ new NODETYPES::Texture(name, uuid, "texture") };
		this->pureNodes.emplace_back(tex, uuid);
		this->textures.emplace_back(tex);
	}
	else if (std::strcmp(type.c_str(), "bump2d"))
	{
		NODETYPES::Bump* bump{ new NODETYPES::Bump(name, uuid, "bump")};
		this->pureNodes.emplace_back(bump, uuid);
		this->bumps.emplace_back(bump);
	}
}
bool DX::loadingObjects()
{
	char* msg{ this->comlib->recv() };
	char* data = msg + sizeof(ComLib::Header);
	
	ComLib::Header header;
	memcpy(&header, msg, sizeof(ComLib::Header));

	switch (header.msgId)
	{
	case ComLib::MSG_TYPE::ALLOCATE:
		switch (header.attrID)
		{
		case ComLib::ATTRIBUTE_TYPE::NODE:
			this->allocateNode(data);
			break;
		default:
			break;
		}
		break;
	case ComLib::MSG_TYPE::DEALLOCATE:
		switch (header.attrID)
		{
		default:
			break;
		}
		break;
	case ComLib::MSG_TYPE::ADDVALUES:
		switch (header.attrID)
		{
		case ComLib::ATTRIBUTE_TYPE::PARENT:
			this->addParent(data);
			break;
		default:
			break;
		}
		break;
	case ComLib::MSG_TYPE::UPDATEVALUES:
		switch (header.attrID)
		{
		case ComLib::ATTRIBUTE_TYPE::TEXPATH:
			this->updateTexture(data);
			break;
		case ComLib::ATTRIBUTE_TYPE::BUMPTEXTURE:
			this->updateBump(data);
			break;
		case ComLib::ATTRIBUTE_TYPE::BUMPSHADER:
			this->appendBumpShader(data);
			break;
		case ComLib::ATTRIBUTE_TYPE::SHCOLOR:
		case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLOR:
		case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCY:
		case ComLib::ATTRIBUTE_TYPE::NORMALMAP:
			this->addMaterialChannels(data, header.attrID);
			break;
		case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
		case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
		case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
			this->addMaterialTextures(data, header.attrID);
			break;
		default:
			break;
		}
		break;
	case ComLib::MSG_TYPE::REMOVEVALUES:
		switch (header.attrID)
		{
		default:
			break;
		}
		break;
	case ComLib::MSG_TYPE::MESSAGE:
		switch (header.attrID)
		{
		case ComLib::ATTRIBUTE_TYPE::EXEND:
			return false;
		default:
			break;
		}
		break;
	default:
		break;
	}
	delete msg;
}
void DX::queryExistingData()
{
	if (comlib->peekExistingMessage())
	{
		bool fetching{ 1 };
		char* msg{ this->comlib->recv() };

		ComLib::Header& header{reinterpret_cast<ComLib::Header&>(msg)};

		switch (header.msgId)
		{
		case ComLib::MSG_TYPE::MESSAGE:
			switch (header.attrID)
			{
			case ComLib::ATTRIBUTE_TYPE::EXSTART:
				while (fetching)
				{
					fetching = loadingObjects();
				}
				break;
			default:
				break;
			}
			break;
		default:
			delete msg;
			queryExistingData();
			break;
		}

		delete msg;
	}
	else
	{
		queryExistingData();
	}
}

HRESULT DX::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT arraySIze, ID3DBlob* VS)
{
	size_t test1 = VS->GetBufferSize();
	LPVOID test2 = VS->GetBufferPointer();

	return this->gDevice->CreateInputLayout(inputDesc, arraySIze, VS->GetBufferPointer(), VS->GetBufferSize(), &this->gVertexLayout);
}
HRESULT DX::CreateVertexShader(LPCWSTR fileName, LPCSTR entryPoint, D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT arraySize)
{
	HRESULT hr {};
	ComPtr<ID3DBlob> error { nullptr };
	ComPtr<ID3DBlob> VS { nullptr };

	hr = D3DCompileFromFile(
		fileName,
		nullptr,
		nullptr,
		entryPoint,
		"vs_5_0",
		D3DCOMPILE_DEBUG,
		NULL,
		&VS,
		&error
	);
	if (SUCCEEDED(hr))
	{
		hr = this->gDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &this->gVertexShader);
		if (SUCCEEDED(hr))
		{
			return this->CreateInputLayout(inputDesc, arraySize, VS.Get());
		}
		else
		{
			OutputDebugStringA("Failed to create vertex shader!");
			return 0x80004005;//E_FAIL
		}
	}
	else
	{
		OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
		return 0x80004005; //E_FAIL
	}
}
HRESULT DX::CreateGeometryShader(LPCWSTR fileName, LPCSTR entryPoint)
{
	HRESULT hr {};
	ComPtr<ID3DBlob> error { nullptr };
	ComPtr<ID3DBlob> GS { nullptr };

	hr = D3DCompileFromFile(
		fileName,
		nullptr,
		nullptr,
		entryPoint,
		"gs_5_0",
		D3DCOMPILE_DEBUG,
		NULL,
		&GS,
		&error
	);
	if (SUCCEEDED(hr))
	{
		return this->gDevice->CreateGeometryShader(GS->GetBufferPointer(), GS->GetBufferSize(), nullptr, &this->gGeometryShader);
	}
	else
	{
		OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
		return 0x80004005;//E_FAIL
	}
}
HRESULT DX::CreatePixelShader(LPCWSTR fileName, LPCSTR entryPoint)
{
	HRESULT hr{};
	ComPtr<ID3DBlob> error { nullptr };
	ComPtr<ID3DBlob> PS { nullptr };

	hr = D3DCompileFromFile(
		fileName,
		nullptr,
		nullptr,
		entryPoint,
		"ps_5_0",
		D3DCOMPILE_DEBUG,
		NULL,
		&PS,
		&error
	);
	if (SUCCEEDED(hr))
	{
		return this->gDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &this->gPixelShader);
	}
	else
	{
		OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
		return 0x80004005;//E_FAIL
	}
}
void DX::CreateShaders()
{
	HRESULT hr;
	
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
	{ "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (FAILED(this->CreateVertexShader(L"MainVertexShader.hlsl", "VS_main", inputDesc, ARRAYSIZE(inputDesc))))
	{
		OutputDebugStringA("Failed to setup vertex shader!");
		exit(-1);
	}
	
	if (FAILED(this->CreateGeometryShader(L"MainGeometryShader.hlsl", "GS_main")))
	{
		OutputDebugStringA("Failed to create geometry shader!");
		exit(-1);
	}

	if (FAILED(this->CreatePixelShader(L"MainPixelShader.hlsl", "PS_main")))
	{
		OutputDebugStringA("Failed to create pixel shader");
		exit(-1);
	}
}

void DX::setViewPort()
{
	D3D11_VIEWPORT vp{
	.TopLeftX = 0,
	.TopLeftY = 0,
	.Width = WIDTH,
	.Height = HEIGHT,
	.MinDepth = 0.f,
	.MaxDepth = 1.f
	};

	this->gDeviceContext->RSSetViewports(1, &vp);
}

HRESULT DX::CreateDeviceSwapchain(HWND* wndHandle)
{
	DXGI_SWAP_CHAIN_DESC swapDesc{};
	swapDesc.BufferCount = 2;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.Width = static_cast<UINT>(WIDTH);
	swapDesc.BufferDesc.Height = static_cast<UINT>(HEIGHT);
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.Windowed = TRUE;
	swapDesc.OutputWindow = *wndHandle;
	swapDesc.SampleDesc.Count = PIXELSAMPLE;
	swapDesc.SampleDesc.Quality = 0;

 return D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_DEBUG,
		nullptr,
		NULL,
		D3D11_SDK_VERSION,
		&swapDesc,
		&this->gSwapchain,
		&this->gDevice,
		nullptr,
		&this->gDeviceContext);
}
HRESULT DX::CreateDepthBuffer(int index)
{
	D3D11_TEXTURE2D_DESC depthBufferDesc {};
	depthBufferDesc.Width = static_cast<UINT>(WIDTH);
	depthBufferDesc.Height = static_cast<UINT>(HEIGHT);
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = PIXELSAMPLE;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.MiscFlags = 0;

	return this->gDevice->CreateTexture2D(&depthBufferDesc, nullptr, &this->gDepthStencil[index]);
}
HRESULT DX::CreateDepthStencil(int index, ID3D11Texture2D* pBackBuffer)
{
	this->gSwapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV {};
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	return this->gDevice->CreateDepthStencilView(this->gDepthStencil[index].Get(), nullptr, &this->gDepthStencilView);
}
HRESULT DX::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasDesc{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE,
		.FrontCounterClockwise = false,
		.DepthBias = 0,
		.DepthBiasClamp = 0,
		.SlopeScaledDepthBias = 0,
		.DepthClipEnable = true,
		.ScissorEnable = false,
		.MultisampleEnable = false,
		.AntialiasedLineEnable = false
	};

	return this->gDevice->CreateRasterizerState(&rasDesc, &this->gRasterizerState);
}

void DX::CreateDirect3DContext(HWND* wndHandle)
{
	HRESULT hr {};
	CoInitialize(nullptr);


	if (SUCCEEDED(this->CreateDeviceSwapchain(wndHandle)))
	{
		ComPtr<ID3D11Texture2D> pBackBuffer[2]{};
		for (int i = 0; i < 2; ++i)
		{
			if (FAILED(this->gSwapchain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer[i].GetAddressOf()))))
			{
				OutputDebugStringA("Failed to querry back buffer!");
				exit(-1);
			}

			if (FAILED(this->CreateDepthBuffer(i)))
			{
				OutputDebugStringA("Failed to create depth buffer!");
				exit(-1);
			}

			if (FAILED(this->CreateDepthStencil(i, pBackBuffer[i].Get())))
			{
				OutputDebugStringA("Failed to create depth stencil!");
				exit(-1);
			}

			if (FAILED(this->gDevice->CreateRenderTargetView(pBackBuffer[i].Get(), nullptr, this->gBackbufferRTV[i].GetAddressOf())))
			{
				OutputDebugStringA("Failed to create render target!");
				exit(-1);
			}

		}
		this->gDeviceContext->OMSetRenderTargets(1, this->gBackbufferRTV[0].GetAddressOf(), this->gDepthStencilView.Get());
		
		//hr = this->gDevice->QueryInterface(IID_PPV_ARGS(&debug));
	}
	else
	{
		OutputDebugStringA("Failed to create swapchain!");
		exit(-1);
	}

	if (FAILED(this->CreateRasterizerState()))
	{
		OutputDebugStringA("Failed to create raster state!");
		exit(-1);
	}
}

void DX::OfflineCreation(HMODULE hModule, HWND* wndHandle)
{
	//send connected (default 1 in ComLib, changed to 0 when crash)
	this->CreateDirect3DContext(wndHandle);
	this->setViewPort();
	this->CreateShaders();

	//Set-up ComLib and fetch data
	this->queryExistingData();
	

}

void DX::Update()
{
}
