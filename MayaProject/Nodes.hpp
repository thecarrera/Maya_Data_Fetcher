#pragma once
#include "Includes.h"
#include "ComLib.h"
#include <array>

using namespace Microsoft::WRL;


namespace NODETYPES
{
	struct MATRICES {
		DirectX::XMMATRIX objectMatrix{};
		DirectX::XMMATRIX worldMatrix{};
	};	

	class Node {
		std::string name{};
		std::string uuid{};
		std::string type{};
		std::vector<std::pair<NODETYPES::Node*, std::string>> referredBy{};
	public:
		virtual ~Node() {};

		std::string getName() const { return name; };
		std::string getUuid() const { return uuid; };
		std::string getType() const { return type; };

		void setName(const std::string& name) { this->name = name; };
		void setUuid(const std::string& uuid) { this->uuid = uuid; };
		void setType(const std::string  type) { this->type = type; };

		void addNewReferenceBy(Node* node, std::string attribute) {
			if (this->referredBy.size() == 0) {
				this->referredBy.emplace_back(node, attribute); 
			}
			else {
				if (std::find(this->referredBy.begin(), this->referredBy.end(), std::pair(node, attribute)) == this->referredBy.end()) {
					this->referredBy.emplace_back(node, attribute);
				}
			}
		}

		void removeReference(std::string uuid);
		void removeReferences();
	};

	class Transform : public Node {
	private:
		std::vector<Node*> parents{};
		std::vector<Node*> children{};

		MATRICES matrices {};
		ComPtr<ID3D11Buffer> matrixBuffer {};

		const DirectX::XMFLOAT4X4 DTFMatrix(const double matrix[4][4]) const {
			DirectX::XMFLOAT4X4 fMatrix{};
			fMatrix._11 = static_cast<float>(matrix[0][0]);
			fMatrix._12 = static_cast<float>(matrix[0][1]);
			fMatrix._13 = static_cast<float>(matrix[0][2]);
			fMatrix._14 = static_cast<float>(matrix[0][3]);

			fMatrix._21 = static_cast<float>(matrix[1][0]);
			fMatrix._22 = static_cast<float>(matrix[1][1]);
			fMatrix._23 = static_cast<float>(matrix[1][2]);
			fMatrix._24 = static_cast<float>(matrix[1][3]);

			fMatrix._31 = static_cast<float>(matrix[2][0]);
			fMatrix._32 = static_cast<float>(matrix[2][1]);
			fMatrix._33 = static_cast<float>(matrix[2][2]);
			fMatrix._34 = static_cast<float>(matrix[2][3]);

			fMatrix._41 = static_cast<float>(matrix[3][0]);
			fMatrix._42 = static_cast<float>(matrix[3][1]);
			fMatrix._43 = static_cast<float>(matrix[3][2]);
			fMatrix._44 = static_cast<float>(matrix[3][3]);

			return fMatrix;
		}

	public:
		Transform(const std::string& name, const std::string& uuid, const std::string type) { 
			this->setName(name); this->setUuid(uuid); this->setType(type); 
			this->matrices.objectMatrix = DirectX::XMMatrixIdentity();
			this->matrices.worldMatrix = DirectX::XMMatrixIdentity();
		};
		~Transform() {};

		void setMatrix(const double matrix[4][4], bool isWorld) {
			const DirectX::XMFLOAT4X4 fMatrix{ this->DTFMatrix(matrix) };
			if (isWorld) {
				this->matrices.worldMatrix = DirectX::XMLoadFloat4x4(&fMatrix);
			}
			else {
				this->matrices.objectMatrix = DirectX::XMLoadFloat4x4(&fMatrix);
			}
		}

		DirectX::XMMATRIX* getWorldMatrix() { return &matrices.worldMatrix; };
		DirectX::XMMATRIX* getObjectMatrix() { return &matrices.objectMatrix; };

		UINT getMatricesStructSize() const { return sizeof(MATRICES); }

		void setupBuffers(ID3D11Device* gDevice) {
			HRESULT hr {};
			D3D11_BUFFER_DESC mBufferDesc {
				.ByteWidth {sizeof(NODETYPES::MATRICES)},
				.Usage {D3D11_USAGE_DYNAMIC},
				.BindFlags {D3D11_BIND_CONSTANT_BUFFER},
				.CPUAccessFlags {D3D11_CPU_ACCESS_WRITE},
				.MiscFlags {},
				.StructureByteStride {}
			};
			D3D11_SUBRESOURCE_DATA mData {.pSysMem {&this->matrices}};
			hr = gDevice->CreateBuffer(&mBufferDesc, &mData, &this->matrixBuffer);
			if (FAILED(hr)) {exit(-1);}
		}

		void resetWorldMatrix() { this->matrices.worldMatrix = DirectX::XMMatrixIdentity(); }
		void resetObjectMatrix() { this->matrices.objectMatrix = DirectX::XMMatrixIdentity(); }

		void addParent(NODETYPES::Node* parent) {
			NODETYPES::Transform* transform {dynamic_cast<NODETYPES::Transform*>(parent)};
			if (transform->children.size() == 0) {
				transform->children.emplace_back(parent);
			}
			else {
				if (std::find(
					transform->children.begin(),
					transform->children.end(),
					parent) == transform->children.end()) 
				{
					transform->children.emplace_back(parent);
				}
			}
			if (parents.size() == 0) {
				parents.emplace_back(parent);
			}
			else {
				if (std::find(parents.begin(), parents.end(), parent) == parents.end()) {
					parents.emplace_back(parent);
				}
			}
		}
		void addChild(NODETYPES::Node* child) { 
			NODETYPES::Transform* transform {dynamic_cast<NODETYPES::Transform*>(child)};
			if (transform->parents.size() == 0) {
				transform->parents.emplace_back(child);
			}
			else {
				if (std::find(
					transform->parents.begin(),
					transform->parents.end(),
					child) == transform->parents.end())
				{
					transform->parents.emplace_back(child);
				}
			}
			if (children.size() == 0) {
				children.emplace_back(child);
			}
			else {
				if (std::find(children.begin(), children.end(), child) == children.end()) {
					parents.emplace_back(child);
				}
			}
		}

		void removeParent(NODETYPES::Node* parent)
		{
			NODETYPES::Transform* transform {dynamic_cast<NODETYPES::Transform*>(parent)};
			for (size_t i = 0; i < transform->children.size(); ++i)
			{
				if (transform->children[i]->getUuid() == this->getUuid())
				{
					transform->children.erase(transform->children.begin() + i);
				}
			}
			for (size_t i = 0; i < this->parents.size(); ++i)
			{
				if (parent->getUuid() == this->parents[i]->getUuid())
				{
					this->parents.erase(this->parents.begin() + i);
				}
			}
		}

		void clearParents();
		void clearChildren();
	};

	class Mesh : public Node
	{
	public:
		//###### OLD! #########
		//struct VERTEX {
		//	size_t pointID {};
		//	size_t normalID {};
		//	size_t uvID {};
		//};
		//
		//struct UVSET {
		//	size_t uvCount {};
		//	std::vector<std::array<float, 2>> uvList {};
		//	
		//	std::vector<size_t> uvIDs {};
		//};
		//#####################

		struct VERTEX
		{
			float point[3]		{};
			float normal[3]		{};
			float tangent[3]	{};
			float bitangent[3]	{};
			float uv[2]			{};
		};

		struct FACE
		{
			std::vector<VERTEX>		vertexList			{};
			std::vector<UINT32>		vertexIDList		{};
			ComPtr<ID3D11Buffer>	gVertexBuffer		{};
			ComPtr<ID3D11Buffer>	gVertexIDBuffer		{};
		};

	private:
		//###### OLD! #########
		//size_t pointsCount {};
		//std::vector<std::array<float, 3>> pointsList {};

		//size_t normalCount {};
		//std::vector<std::array<float, 3>> normalList {};

		//size_t uvSetCount {};
		//std::vector<std::pair<UVSET, std::string>> uvSets{}; // set | name
		//
		//std::vector<size_t>		vertexList {}; //IDs
		//#####################

		std::vector<FACE> faces {};
		std::vector<NODETYPES::Node*> shadingEngines {};
		NODETYPES::Node* transformer{};

	public:
		Mesh(const std::string& name, const std::string& uuid, const std::string type) {
			this->setName(name);
			this->setUuid(uuid);
			this->setType(type);
		};
		~Mesh() {};

		//###### OLD! #########
		//void updateList(size_t actualPos, int iteratedPos, double container[10000][3], ComLib::ATTRIBUTE_TYPE attr) {
		//	int counter {-1};
		//	if (attr == ComLib::ATTRIBUTE_TYPE::VERTEX)
		//	{
		//		for (size_t i = (actualPos - iteratedPos); i < actualPos; ++i)
		//		{
		//			++counter;
		//			this->pointsList[i][0] = static_cast<float>(container[counter][0]);
		//			this->pointsList[i][1] = static_cast<float>(container[counter][1]);
		//			this->pointsList[i][2] = static_cast<float>(container[counter][2]);
		//		}
		//	}
		//	else if (attr == ComLib::ATTRIBUTE_TYPE::NORMAL)
		//	{
		//		for (size_t i = (actualPos - iteratedPos); i < actualPos; ++i)
		//		{
		//			++counter;
		//			this->normalList[i][0] = static_cast<float>(container[counter][0]);
		//			this->normalList[i][1] = static_cast<float>(container[counter][1]);
		//			this->normalList[i][2] = static_cast<float>(container[counter][2]);
		//		}
		//	}
		//}
		//void updateList(size_t actualPos, int iteratedPos, size_t setIndex, double container[10000][2])
		//{
		//	int counter{ -1 };
		//	for (size_t i = (actualPos - iteratedPos); i < actualPos; ++i)
		//	{
		//		++counter;
		//		this->uvSets[setIndex].first.uvList[i][0] = static_cast<float>(container[counter][0]);
		//		this->uvSets[setIndex].first.uvList[i][1] = static_cast<float>(container[counter][1]);
		//	}
		//}
		//HRESULT allocateList(size_t count, ID3D11Device* gDevice, UVSET* uvSet, ComLib::ATTRIBUTE_TYPE attr)
		//{
		//	switch (attr)
		//	{
		//	case ComLib::ATTRIBUTE_TYPE::UVSET:
		//		if (count > uvSet->uvList.capacity() || (count <= uvSet->uvList.capacity() / 2)) {
		//			uvSet->uvList.resize(count);
		//			return S_OK;
		//		}
		//		else { return E_ABORT; }
		//		break;
		//	case ComLib::ATTRIBUTE_TYPE::UVID:
		//		
		//		break;
		//	}
		//}
		//
		//UVSET* getUVSet(size_t index)
		//{
		//	if ((index >= 0) && (index <= this->uvSetCount))
		//		return &this->uvSets[index].first;			
		//	else
		//		return nullptr;
		//}
		//UVSET* getUVSet(std::string uvSetName)
		//{
		//	for (size_t i = 0; i < this->uvSetCount; ++i) {
		//		if (this->uvSets[i].second == uvSetName)
		//			return &this->uvSets[i].first;
		//	} 
		//	return nullptr;
		//}
		//
		//void setUVSetName(const size_t index, const std::string name) { this->uvSets[index].second = name; }
		//#####################
		void allocateVertices(ID3D11Device* gDevice, UINT faceIndex, UINT vertexCount, UINT vertexIDCount)
		{
			this->faces[faceIndex].vertexList.resize(vertexCount);
			this->faces[faceIndex].vertexIDList.resize(vertexIDCount);

			// Vertex Buffer
				if (this->faces[faceIndex].gVertexBuffer.Get()) {
					this->faces[faceIndex].gVertexBuffer.Get()->Release();
					this->faces[faceIndex].gVertexBuffer.Reset();
			}

			D3D11_BUFFER_DESC bufferDesc{
			.ByteWidth{UINT(sizeof(VERTEX) * this->faces[faceIndex].vertexList.capacity())},
			.Usage{D3D11_USAGE_DYNAMIC},
			.BindFlags{D3D11_BIND_VERTEX_BUFFER},
			.CPUAccessFlags{D3D11_CPU_ACCESS_WRITE}
			};
			
			D3D11_SUBRESOURCE_DATA data{
			.pSysMem {this->faces[faceIndex].vertexList.data()}
			};
			HRESULT hr{ gDevice->CreateBuffer(&bufferDesc, &data, this->faces[faceIndex].gVertexBuffer.GetAddressOf()) };

			//Vertex ID Buffer
			if (this->faces[faceIndex].gVertexIDBuffer.Get()) {
				this->faces[faceIndex].gVertexIDBuffer.Get()->Release();
				this->faces[faceIndex].gVertexIDBuffer.Reset();
			}

			
			bufferDesc.ByteWidth = UINT(sizeof(UINT32) * this->faces[faceIndex].vertexIDList.capacity());
			bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			data.pSysMem = this->faces[faceIndex].vertexIDList.data();
			
			hr = gDevice->CreateBuffer(&bufferDesc, &data, this->faces[faceIndex].gVertexIDBuffer.GetAddressOf());
		}
		void allocateFaces(UINT faceCount)
		{
			this->faces.resize(faceCount);
		}
		
		//HRESULT allocateList(const size_t count, ID3D11Device* gDevice, const ComLib::ATTRIBUTE_TYPE attr) {
		//	if (attr == ComLib::ATTRIBUTE_TYPE::VERTEX)
		//	{
		//		if (count > this->vertexList.capacity() || count < this->vertexList.capacity())
		//		{
		//			//size_t multiplier {static_cast<size_t>(std::ceil(count / 10000.0f))};
		//			//this->vertexList.reserve(10000 * multiplier);
		//		
		//			this->vertexList.resize(count);
		//		}
		//		if (this->gVertexBuffer.Get()) {
		//			this->gVertexBuffer.Get()->Release();
		//			this->gVertexBuffer.Reset();
		//		}
		//		D3D11_BUFFER_DESC bufferDesc{
		//		.ByteWidth{UINT(sizeof(VERTEX) * this->vertexList.capacity())},
		//		.Usage{D3D11_USAGE_DYNAMIC},
		//		.BindFlags{D3D11_BIND_VERTEX_BUFFER},
		//		.CPUAccessFlags{D3D11_CPU_ACCESS_WRITE}
		//		};

		//		D3D11_SUBRESOURCE_DATA data{
		//		.pSysMem {this->vertexList.data()}
		//		};
		//		HRESULT hr{ gDevice->CreateBuffer(&bufferDesc, &data, this->gVertexBuffer.GetAddressOf()) };
		//		if (SUCCEEDED(hr))
		//			return S_OK;
		//		else
		//			return E_FAIL;
		//	}
		//	return E_INVALIDARG;
		//}

		void updateList(const UINT faceIndex, const UINT actualPos, const UINT iteratedPos, const std::vector<VERTEX> container) {
			int counter{ -1 };
			for (UINT i = (actualPos - iteratedPos) - 1; i < actualPos; ++i) {
				++counter;
				this->faces[faceIndex].vertexList[i] = container[counter];
			}
		}
		void updateList(const UINT faceIndex, const size_t actualPos, const int iteratedPos, const std::vector<UINT32> container)
		{
			int counter{ -1 };
			for (size_t i = (actualPos - iteratedPos) - 1; i < actualPos; ++i) {
				++counter;
				this->faces[faceIndex].vertexIDList[i] = container[counter];
			}
		}

		void setTransform(NODETYPES::Node* node) {
			node->addNewReferenceBy(this, "transform");
			this->transformer = node;
		}
		void setShadingEngines(const std::vector<NODETYPES::Node*> shadingEngines)
		{
			this->shadingEngines = shadingEngines;
		}

		DirectX::XMMATRIX* getTransformWorld() {
			return dynamic_cast<NODETYPES::Transform*>(this->transformer)->getWorldMatrix();
		}

		UINT getFaceCount() const { return static_cast<UINT>(this->faces.size()); }
		UINT32 getVertexCount(UINT faceIndex) { return static_cast<UINT32>(this->faces[faceIndex].vertexIDList.size()); }
		std::vector<VERTEX> getVertexList(UINT faceIndex) { return this->faces[faceIndex].vertexList; };
		std::vector<UINT32> getVertexIDs(UINT faceIndex) { return this->faces[faceIndex].vertexIDList; }
		ID3D11Buffer* getVertexBuffer(UINT faceIndex) { return this->faces[faceIndex].gVertexBuffer.Get(); }
		ID3D11Buffer* getVertexIDBuffer(UINT faceIndex) { return this->faces[faceIndex].gVertexIDBuffer.Get();	}

		ID3D11ShaderResourceView* getDiffuseBuffer();
		ID3D11ShaderResourceView* getNormalBuffer();

		void updateVertexListToBuffer(UINT faceIndex, ID3D11DeviceContext* gDeviceContext)
		{
			D3D11_MAPPED_SUBRESOURCE dataPtr {};
			gDeviceContext->Map(this->faces[faceIndex].gVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
			memcpy(dataPtr.pData, this->faces[faceIndex].vertexList.data(), sizeof(NODETYPES::Mesh::VERTEX) * this->faces[faceIndex].vertexList.size());
			gDeviceContext->Unmap(this->faces[faceIndex].gVertexBuffer.Get(), 0);
		}
		void updateVertexIDToBuffer(UINT faceIndex, ID3D11DeviceContext* gDeviceContext)
		{
			D3D11_MAPPED_SUBRESOURCE dataPtr{};
			gDeviceContext->Map(this->faces[faceIndex].gVertexIDBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
			memcpy(dataPtr.pData, this->faces[faceIndex].vertexIDList.data(), sizeof(UINT32) * this->faces[faceIndex].vertexIDList.size());
			gDeviceContext->Unmap(this->faces[faceIndex].gVertexIDBuffer.Get(), 0);
		}

		bool existShader();
		bool existDiffuse();
		bool existNormal();

		void clearShadingEngines();
		void clearTransformReference();

		//Used by nodes referenced to this node
		void removeTransformReference() { 
			if (transformer)
			{
				this->transformer->removeReference(this->getUuid());
				this->transformer = nullptr; 
			}
		}
		void removeShadingEngine(std::string uuid) {
			for (size_t i = 0; i < this->shadingEngines.size(); ++i) {
				if (this->shadingEngines[i]->getUuid() == uuid)
				{
					this->shadingEngines.erase(shadingEngines.begin() + i);
				}
			}
		}
	};

	class PointLight : public Node
	{
	public:
		struct POINTDATA {
			float intensity{};
			float color[3]{};
			DirectX::XMMATRIX worldMat {};
		};
	private:
		POINTDATA pointData {};
		ComPtr<ID3D11Buffer> pointBuffer {};

		NODETYPES::Node* transformer{};

	public:
		
		PointLight(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~PointLight() {};

		void setIntensity(const float intensity) { this->pointData.intensity = intensity; };
		void setColor(const float color[3]) {
			this->pointData.color[0] = static_cast<float>(color[0]);
			this->pointData.color[1] = static_cast<float>(color[1]);
			this->pointData.color[2] = static_cast<float>(color[2]);
		};
		void setupBuffers(ID3D11Device* gDevice) {
			HRESULT hr{};
			D3D11_BUFFER_DESC mBufferDesc{
				.ByteWidth {sizeof(NODETYPES::PointLight::POINTDATA)},
				.Usage {D3D11_USAGE_DYNAMIC},
				.BindFlags {D3D11_BIND_CONSTANT_BUFFER},
				.CPUAccessFlags {D3D11_CPU_ACCESS_WRITE},
				.MiscFlags {},
				.StructureByteStride {}
			};
			D3D11_SUBRESOURCE_DATA mData {.pSysMem {&this->pointData}};
			hr = gDevice->CreateBuffer(&mBufferDesc, &mData, &this->pointBuffer);
			if (FAILED(hr)) {exit(-1);}
		}

		void setTransform(NODETYPES::Node* node) { 
			node->addNewReferenceBy(this, "pointLight");
			this->transformer = node; };

		POINTDATA* getPointData() {
			return &this->pointData;
		}
		DirectX::XMMATRIX* getTransformWorld() {
			if (this->transformer)
			{
				return dynamic_cast<NODETYPES::Transform*>(this->transformer)->getWorldMatrix();
			}
			else
			{
				this->pointData.worldMat = DirectX::XMMatrixIdentity();
				return &pointData.worldMat;
			}
		}

		void clearTransformReference() {
			dynamic_cast<NODETYPES::Transform*>(this->transformer)->removeReference(this->getUuid());
		}

		//For References
		void removeTransformReference() { 
			this->transformer->removeReference(this->getUuid());
			this->transformer = nullptr; 
		}
	};

	class Camera : public Node
	{
	private:
		NODETYPES::Node* viewMatrix{};
		DirectX::XMMATRIX projectionMatrix{};
		ComPtr<ID3D11Buffer> projectionBuffer {};

	public:
		Camera(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Camera() {};

		void setProjectionMatrix(const double projMat[4][4]) {
			DirectX::XMFLOAT4X4 tempMat{
			static_cast<float>(projMat[0][0]), static_cast<float>(projMat[0][1]), static_cast<float>(projMat[0][2]), static_cast<float>(projMat[0][3]),
			static_cast<float>(projMat[1][0]), static_cast<float>(projMat[1][1]), static_cast<float>(projMat[1][2]), static_cast<float>(projMat[1][3]),
			static_cast<float>(projMat[2][0]), static_cast<float>(projMat[2][1]), static_cast<float>(projMat[2][2]), static_cast<float>(projMat[2][3]),
			static_cast<float>(projMat[3][0]), static_cast<float>(projMat[3][1]), static_cast<float>(projMat[3][2]), static_cast<float>(projMat[3][3])
			};
			this->projectionMatrix = DirectX::XMLoadFloat4x4(&tempMat);
		};
		void setupBuffers(ID3D11Device* gDevice)
		{
			HRESULT hr{};
			D3D11_BUFFER_DESC mBufferDesc{
				.ByteWidth {sizeof(DirectX::XMMATRIX)},
				.Usage {D3D11_USAGE_DYNAMIC},
				.BindFlags {D3D11_BIND_CONSTANT_BUFFER},
				.CPUAccessFlags {D3D11_CPU_ACCESS_WRITE},
				.MiscFlags {},
				.StructureByteStride {}
			};
			D3D11_SUBRESOURCE_DATA mData {.pSysMem {&this->projectionMatrix}};
			hr = gDevice->CreateBuffer(&mBufferDesc, &mData, &this->projectionBuffer);
			if (FAILED(hr)) { exit(-1); }
		}
		void setViewMatrix(NODETYPES::Node* node) { 
			node->addNewReferenceBy(this, "viewMatrix");
			this->viewMatrix = node; 
		}

		DirectX::XMMATRIX* getProjectionMatrix() { return &this->projectionMatrix; };
		DirectX::XMMATRIX* getViewMatix() {
			if (this->viewMatrix == nullptr)
			{
				std::cout << "No view matrix transform";
				return nullptr;
			}
			else
			{
				return dynamic_cast<NODETYPES::Transform*>(this->viewMatrix)->getWorldMatrix();
			}
		};

		void clearViewMatrixReference() {
			viewMatrix->removeReference(this->getUuid());
			viewMatrix = nullptr;
		}

		void removeViewMatrixReference() { 
			this->viewMatrix->removeReference(this->getUuid());
			this->viewMatrix = nullptr;
		}
	};

	class Texture : public Node
	{
	private:
		bool textureExist {};
		std::string filePath {};

		ComPtr<ID3D11ShaderResourceView>	gTextureSRV	{};
		ComPtr<ID3D11Resource>				gTexture	{};

	public:
		Texture(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Texture() {this->gTexture.Get()->Release(); this->gTextureSRV.Get()->Release();};

		HRESULT InitTexture(ID3D11Device* const gDevice, ID3D11DeviceContext* const gDeviceContext, const std::string filePath) {
			wchar_t fileName[200]{};
			for (size_t i = 0; i < filePath.length(); ++i)
			{
				fileName[i] = filePath[i];
			}
			return DirectX::CreateWICTextureFromFile(gDevice, fileName, &this->gTexture, &this->gTextureSRV, NULL);
		}

		bool getTextureExist() const { return this->textureExist; };
		std::string getFilePath() const { return this->filePath; };
		ID3D11ShaderResourceView* getTextureSRV() const { return this->gTextureSRV.Get(); };

		void toggleExistTexture(const bool flip) { this->textureExist = flip; };
		void setFilePath(const std::string& filePath) { this->filePath = filePath; };
	};

	class Bump : public Node
	{
	private:
		std::vector<NODETYPES::Node*> textures;

	public:
		Bump(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Bump() {};

		void setTextures(const std::vector<NODETYPES::Node*> textures) { this->textures = textures; };
		//void addShader(NODETYPES::Node* const shaderNode) { this->shaders.emplace_back(shaderNode); };

		std::vector<NODETYPES::Node*> getTextures() const { return this->textures; };
		NODETYPES::Texture* getTextureAt(const int index) const { return dynamic_cast<NODETYPES::Texture*>(this->textures[index]); };

		bool existNormal() {
			if (this->textures.size() > 0) {
				return dynamic_cast<NODETYPES::Texture*>(this->textures[0])->getTextureExist();
			}
			else {
				return false;
			}
		}
		ID3D11ShaderResourceView* getNormalSRV() {
			return dynamic_cast<NODETYPES::Texture*>(this->textures[0])->getTextureSRV();
		}

		size_t findTextureIndex(const std::string textureUuid) {
			for (size_t i = 0; i < this->textures.size(); ++i) {
				if (this->textures[i]->getUuid() == textureUuid) { return i; }
				else { return 0; }
			}
		};
		HRESULT findTexture(const std::string textureUuid, NODETYPES::Texture* texture) {
			for (size_t i = 0; i < this->textures.size(); ++i) {
				if (this->textures[i]->getUuid() == textureUuid) { texture = dynamic_cast<NODETYPES::Texture*>(this->textures[i]); return S_OK; }
				else { return E_FAIL; }
			}
		};

		void removeTexture(std::string uuid) {
			for (size_t i = 0; i < this->textures.size(); ++i) {
				if (this->textures[i]->getUuid() == uuid)
				{
					this->textures.erase(textures.begin() + i);
				}
			}
		}

		void clearTextures() {
			for (size_t i = 0; i < this->textures.size(); ++i)
			{
				textures[i]->removeReference(this->getUuid());
			}
		}
	};

	class Lambert : public Node
	{
	private:
		bool diffuseConnected{};
		DirectX::XMFLOAT3 color{};
		std::vector<NODETYPES::Node*> diffuseMaps{};

		bool colorMapConnected{};
		DirectX::XMFLOAT3 ambientColor{};
		std::vector<NODETYPES::Node*> colorMaps{};

		bool transparancyMapConnected{};
		DirectX::XMFLOAT3 transparancy{};
		std::vector<NODETYPES::Node*> transparancyMaps{};

		bool normalMapConnected{};
		std::vector<NODETYPES::Node*> normalMaps{};

	public:
		Lambert(std::string& name, std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Lambert() {};

		void setChannels(float const values[3], ComLib::ATTRIBUTE_TYPE attribute) {
			switch (attribute)
			{
			case ComLib::ATTRIBUTE_TYPE::SHCOLOR:
				this->color.x = values[0]; this->color.y = values[1]; this->color.z = values[2];
				break;
			case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLOR:
				this->ambientColor.x = values[0]; this->ambientColor.y = values[1]; this->ambientColor.z = values[2];
				break;
			case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCY:
				this->transparancy.x = values[0]; this->transparancy.y = values[1]; this->transparancy.z = values[2];
				break;
			}
		};
		void setTextureConnected(bool connection, ComLib::ATTRIBUTE_TYPE attribute) {
			switch (attribute)
			{
			case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
				this->diffuseConnected = connection;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
				this->colorMapConnected = connection;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
				this->transparancyMapConnected = connection;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHNORMALMAP:
				this->normalMapConnected = connection;
			}
		};
		void setTextureMaps(std::vector<NODETYPES::Node*> maps, ComLib::ATTRIBUTE_TYPE attribute) {
			switch (attribute)
			{
			case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
				this->diffuseMaps = maps;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
				this->colorMaps = maps;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
				this->transparancyMaps = maps;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHNORMALMAP:
				this->normalMaps = maps;
			}
		};

		bool existDiffuse() {
			if (this->diffuseMaps.size() > 0)
			{
				return dynamic_cast<NODETYPES::Texture*>(this->diffuseMaps[0])->getTextureExist();
			}
			else
			{
				return false;
			}
		}
		bool existBump() {
			if (this->normalMaps.size() > 0)
			{
				return dynamic_cast<NODETYPES::Bump*>(this->normalMaps[0])->existNormal();
			}
			else
			{
				return false;
			}
		}
		ID3D11ShaderResourceView* getDiffuseMap() {
			return dynamic_cast<NODETYPES::Texture*>(this->diffuseMaps[0])->getTextureSRV();
		}
		ID3D11ShaderResourceView* getNormalMap() {
			return dynamic_cast<NODETYPES::Bump*>(this->normalMaps[0])->getNormalSRV();
		}

		void removeDiffuseMap(std::string uuid) {
			for (size_t i = 0; i < this->diffuseMaps.size(); ++i) {
				if (this->diffuseMaps[i]->getUuid() == uuid)
				{
					this->diffuseMaps.erase(diffuseMaps.begin() + i);
				}
			}
		}
		void removeColorMap(std::string uuid) {
			for (size_t i = 0; i < this->colorMaps.size(); ++i) {
				if (this->colorMaps[i]->getUuid() == uuid)
				{
					this->colorMaps.erase(colorMaps.begin() + i);
				}
			}
		}
		void removeTransparancyMap(std::string uuid) {
			for (size_t i = 0; i < this->transparancyMaps.size(); ++i) {
				if (this->transparancyMaps[i]->getUuid() == uuid)
				{
					this->transparancyMaps.erase(transparancyMaps.begin() + i);
				}
			}
		}
		void removeNormalMap(std::string uuid) {
			for (size_t i = 0; i < this->normalMaps.size(); ++i) {
				if (this->normalMaps[i]->getUuid() == uuid)
				{
					this->normalMaps.erase(normalMaps.begin() + i);
				}
			}
		}

		void clearDiffuseMaps() {
			for (size_t i = 0; i < this->diffuseMaps.size(); ++i)
			{
				this->diffuseMaps[i]->removeReference(this->getUuid());
			}
		}
		void clearColorMaps() {
			for (size_t i = 0; i < this->colorMaps.size(); ++i)
			{
				this->colorMaps[i]->removeReference(this->getUuid());
			}
		}
		void clearTransparancyMaps() {
			for (size_t i = 0; i < this->transparancyMaps.size(); ++i)
			{
				this->transparancyMaps[i]->removeReference(this->getUuid());
			}
		}
		void clearNormalMaps() {
			for (size_t i = 0; i < this->normalMaps.size(); ++i)
			{
				this->normalMaps[i]->removeReference(this->getUuid());
			}
		}
	};

	class Blinn : public Node
	{
	private:
		bool diffuseConnected{};
		DirectX::XMFLOAT3 color{};
		std::vector<NODETYPES::Node*> diffuseMaps{};

		bool colorMapConnected{};
		DirectX::XMFLOAT3 ambientColor{};
		std::vector<NODETYPES::Node*> colorMaps{};

		bool transparancyMapConnected{};
		DirectX::XMFLOAT3 transparancy{};
		std::vector<NODETYPES::Node*> transparancyMaps{};

		bool normalMapConnected{};
		std::vector<NODETYPES::Node*> normalMaps{};

		//bool specularMapConnected{};
		//DirectX::XMFLOAT3 specular{};
		//std::vector<NODETYPES::Node*> specularMaps{};

		std::vector<NODETYPES::Node*> shadingEngines{};

	public:
		Blinn(std::string& name, std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Blinn() {};

		void setChannels(float const values[3], ComLib::ATTRIBUTE_TYPE attribute) {
			switch (attribute)
			{
			case ComLib::ATTRIBUTE_TYPE::SHCOLOR:
				this->color.x = values[0]; this->color.y = values[1]; this->color.z = values[2];
				break;
			case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLOR:
				this->ambientColor.x = values[0]; this->ambientColor.y = values[1]; this->ambientColor.z = values[2];
				break;
			case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCY:
				this->transparancy.x = values[0]; this->transparancy.y = values[1]; this->transparancy.z = values[2];
				break;
				//ADD SUPPORT FOR SPECULAR
				//---
				//---
			}
		};
		void setTextureConnected(bool connection, ComLib::ATTRIBUTE_TYPE attribute) {
			switch (attribute)
			{
			case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
				this->diffuseConnected = connection;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
				this->colorMapConnected = connection;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
				this->transparancyMapConnected = connection;
				break;
				//ADD SUPPORT FOR SPECULAR
				//---
				//---
			}
		};
		void setTextureMaps(std::vector<NODETYPES::Node*> maps, ComLib::ATTRIBUTE_TYPE attribute) {
			switch (attribute)
			{
			case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
				this->diffuseMaps = maps;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
				this->colorMaps = maps;
				break;
			case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
				this->transparancyMaps = maps;
				break;
				//ADD SUPPORT FOR SPECULAR
				//---
				//---
			}
		};

		bool existDiffuse() {
			if (this->diffuseConnected)
			{
				return dynamic_cast<NODETYPES::Texture*>(this->diffuseMaps[0])->getTextureExist();
			}
			else
			{
				return false;
			}
		}
		bool existBump() {
			if (this->normalMaps.size() > 0)
			{
				return dynamic_cast<NODETYPES::Bump*>(this->normalMaps[0])->existNormal();
			}
			else
			{
				return false;
			}
		}
		ID3D11ShaderResourceView* getDiffuseMap() {
			return dynamic_cast<NODETYPES::Texture*>(this->diffuseMaps[0])->getTextureSRV();
		}
		ID3D11ShaderResourceView* getNormalMap() {
			return dynamic_cast<NODETYPES::Texture*>(this->normalMaps[0])->getTextureSRV();
		}

		void removeDiffuseMap(std::string uuid) {
			for (size_t i = 0; i < this->diffuseMaps.size(); ++i) {
				if (this->diffuseMaps[i]->getUuid() == uuid)
				{
					this->diffuseMaps.erase(diffuseMaps.begin() + i);
				}
			}
		}
		void removeColorMap(std::string uuid) {
			for (size_t i = 0; i < this->colorMaps.size(); ++i) {
				if (this->colorMaps[i]->getUuid() == uuid)
				{
					this->colorMaps.erase(colorMaps.begin() + i);
				}
			}
		}
		void removeTransparancyMap(std::string uuid) {
			for (size_t i = 0; i < this->transparancyMaps.size(); ++i) {
				if (this->transparancyMaps[i]->getUuid() == uuid)
				{
					this->transparancyMaps.erase(transparancyMaps.begin() + i);
				}
			}
		}
		void removeNormalMap(std::string uuid) {
			for (size_t i = 0; i < this->normalMaps.size(); ++i) {
				if (this->normalMaps[i]->getUuid() == uuid)
				{
					this->normalMaps.erase(normalMaps.begin() + i);
				}
			}
		}

		void clearDiffuseMaps() {
			for (size_t i = 0; i < this->diffuseMaps.size(); ++i)
			{
				this->diffuseMaps[i]->removeReference(this->getUuid());
			}
		}
		void clearColorMaps() {
			for (size_t i = 0; i < this->colorMaps.size(); ++i)
			{
				this->colorMaps[i]->removeReference(this->getUuid());
			}
		}
		void clearTransparancyMaps() {
			for (size_t i = 0; i < this->transparancyMaps.size(); ++i)
			{
				this->transparancyMaps[i]->removeReference(this->getUuid());
			}
		}
		void clearNormalMaps() {
			for (size_t i = 0; i < this->normalMaps.size(); ++i)
			{
				this->normalMaps[i]->removeReference(this->getUuid());
			}
		}

	};

	class ShadingEngine : public Node
	{
	private:
		std::vector<NODETYPES::Node*> materials {};

	public:
		ShadingEngine(std::string& name, std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~ShadingEngine() {};

		void setMaterials(std::vector<NODETYPES::Node*> materials) { this->materials = materials; };

		std::vector<NODETYPES::Node*> getMaterials() const { return this->materials; };
		NODETYPES::Node* getMaterialAt(const int index) const { return this->materials[index]; };

		size_t findMaterialIndex(const std::string materialUuid) {
			for (size_t i = 0; i < this->materials.size(); ++i) {
				if (this->materials[i]->getUuid() == materialUuid) { return i; }
				else { return 0; }
			}
		};
		HRESULT findMaterial(std::string materialUuid, NODETYPES::Node* node) {
			for (size_t i = 0; i < this->materials.size(); ++i) {
				if (this->materials[i]->getUuid() == materialUuid) { node = this->materials[i]; return S_OK; }
				else { return E_FAIL; }
			}
		};

		bool getMaterialExist() {
			if (this->materials.size() > 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		bool getDiffuseExist() {
			if (this->materials[0]->getType() == "lambert")
			{
				return dynamic_cast<NODETYPES::Lambert*>(this->materials[0])->existDiffuse();
			}
			else if (this->materials[0]->getType() == "blinn")
			{
				return dynamic_cast<NODETYPES::Blinn*>(this->materials[0])->existDiffuse();
			}
			return false;
		}
		bool getNormalExist() {
			if (this->materials[0]->getType() == "lambert")
			{
				return dynamic_cast<NODETYPES::Lambert*>(this->materials[0])->existBump();
			}
			else if (this->materials[0]->getType() == "blinn")
			{
				return dynamic_cast<NODETYPES::Blinn*>(this->materials[0])->existBump();
			}
			return false;
		}

		ID3D11ShaderResourceView* getDiffuseTexture() {
			if (this->materials[0]->getType() == "lambert")
			{
				return dynamic_cast<NODETYPES::Lambert*>(this->materials[0])->getDiffuseMap();
			}
			else if(this->materials[0]->getType() == "blinn")
			{
				return dynamic_cast<NODETYPES::Blinn*>(this->materials[0])->getDiffuseMap();
			}
			return nullptr;
		}
		ID3D11ShaderResourceView* getNormalTexture() {
			if (this->materials[0]->getType() == "lambert")
			{
				return dynamic_cast<NODETYPES::Lambert*>(this->materials[0])->getNormalMap();
			}
			else if (this->materials[0]->getType() == "blinn")
			{
				return dynamic_cast<NODETYPES::Blinn*>(this->materials[0])->getNormalMap();
			}
			return nullptr;
		}

		void clearMaterials() {
			for (size_t i = 0; i < this->materials.size(); ++i)
			{
				if (this->materials[i]->getType() == "lambert") {
					NODETYPES::Lambert* lambert {dynamic_cast<NODETYPES::Lambert*>(this->materials[i])};
					lambert->removeReference(this->getUuid());
				}
				else if (this->materials[i]->getType() == "blinn")
				{
					NODETYPES::Blinn* blinn {dynamic_cast<NODETYPES::Blinn*>(this->materials[i])};
					blinn->removeReference(this->getUuid());
				}
			}
			this->materials.clear();
		}

		void removeMaterial(std::string uuid) {
			for (size_t i = 0; i < this->materials.size(); ++i) {
				if (this->materials[i]->getUuid() == uuid)
				{
					this->materials.erase(materials.begin() + i);
				}
			}
		}
	};
}