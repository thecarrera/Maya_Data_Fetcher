#pragma once
#include "Includes.h"
#include "ComLib.h"
#include <array>

using namespace Microsoft::WRL;

// ###### NOTES ############
// * Abstract further by adding class attribute. class Node{ std::vector<attributes> };
//   This way, class member variables and functions are irrelevant.
//   Attribute class contains 
//   - void* Data
//	 - bool existData
//   - type
//   - converters to raw data or subclasses like mesh. (mesh getAsMesh() { dynamic_cast<mesh>(Data) } | void* getAsObject() | float[3] getAsFloat3()
//	 (Maybe shared_ptr with member variables from actual class objects? attribute -> void* data = shared_ptr<type> data

namespace NODETYPES
{
	class Node
	{
		std::string name {};
		std::string uuid {};
		std::string type {};

		std::vector<Node*> parents	{};
		std::vector<Node*> children	{};
	public:
		virtual ~Node() {};
	
		std::string getName() const		{ return name; };
		std::string getUuid() const		{ return uuid; };
		std::string getType() const		{ return type; };
		Node* getParent(const int index) const { 
			if (index <= parents.size() && index >= 0ULL) 
			{ return parents[index]; } 
			else 
			{ return nullptr; } 
		};

		void setName(const std::string& name) { this->name = name; };
		void setUuid(const std::string& uuid) { this->uuid = uuid; };
		void setType(const std::string  type) { this->type = type; };
	
		void addParent(Node* const parent) { this->parents.emplace_back(parent); };
		void removeParent(const std::string* parentUuid) {};
	};
	
	class Transform : public Node
	{
	private:
		DirectX::XMMATRIX matrix {};

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
		Transform(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Transform() {};

		void setMatrix(const double matrix[4][4]) {
			const DirectX::XMFLOAT4X4 fMatrix{ this->DTFMatrix(matrix) };
			this->matrix = DirectX::XMLoadFloat4x4(&fMatrix);
		}
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
			double point[3]{};
			double normal[3]{};
			double uv[2]{};
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

		std::vector<VERTEX> vertexList{};
		std::vector<size_t> vertexIDs{};
		ComPtr<ID3D11Buffer>	gVertexBuffer{};
		ComPtr<ID3D11Buffer>	gVertexIDBuffer{};

		std::vector<NODETYPES::Node*> shadingEngines;

	public:

		Mesh(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
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

		HRESULT allocateList(const size_t count, ID3D11Device* gDevice, const ComLib::ATTRIBUTE_TYPE attr) {
			if (attr == ComLib::ATTRIBUTE_TYPE::VERTEX)
			{
				if (count > this->vertexList.capacity() || (count <= this->vertexList.capacity() / 2))
				{
					this->vertexList.resize(count);

					if (this->gVertexBuffer.Get()) {
						this->gVertexBuffer.Get()->Release();
						this->gVertexBuffer.Reset();
					}
					D3D11_BUFFER_DESC bufferDesc {
					.ByteWidth{UINT(sizeof(VERTEX) * count)},
					.Usage{D3D11_USAGE_DEFAULT},
					.BindFlags{D3D11_BIND_VERTEX_BUFFER}
					};

					D3D11_SUBRESOURCE_DATA data {
					.pSysMem {this->vertexList.data()}
					};
					HRESULT hr{ gDevice->CreateBuffer(&bufferDesc, &data, this->gVertexBuffer.GetAddressOf()) };
					if (SUCCEEDED(hr))
						return S_OK;
					else
						return E_FAIL;
				}
				else { return E_ABORT; }
			}
			else if (attr == ComLib::ATTRIBUTE_TYPE::VERTEXID)
			{
				if (count > this->vertexIDs.capacity() || (count <= this->vertexIDs.capacity() / 2))
				{
					this->vertexIDs.resize(count);

					if (this->gVertexIDBuffer.Get()) {
						this->gVertexIDBuffer.Get()->Release();
						this->gVertexIDBuffer.Reset();
					}

					D3D11_BUFFER_DESC bufferDesc {
						.ByteWidth { UINT(STSIZE * count)},
						.Usage { D3D11_USAGE_IMMUTABLE },
						.BindFlags { D3D11_BIND_INDEX_BUFFER }
					};

					D3D11_SUBRESOURCE_DATA data{
					.pSysMem {this->vertexList.data()}
					};
					HRESULT hr { gDevice->CreateBuffer(&bufferDesc, &data, this->gVertexIDBuffer.GetAddressOf()) };
					if (SUCCEEDED(hr))
						return S_OK;
					else
						return E_FAIL;
				}
				else { return E_ABORT; }


			}
			//else if (attr == ComLib::ATTRIBUTE_TYPE::NORMAL)
			//{
			//	if (count > this->normalList.capacity() || (count <= this->normalList.capacity() / 2))
			//	{
			//		this->normalList.resize(count);
			//		return S_OK;
			//	}
			//	else { return E_ABORT; }
			//}
			//else if (attr == ComLib::ATTRIBUTE_TYPE::UVSETS)
			//{
			//	if (count > this->uvSets.capacity() || (count <= this->normalList.capacity() / 2))
			//	{
			//		this->uvSets.resize(count);
			//		return S_OK;
			//	}
			//	else { return E_ABORT; }
			//}
			return E_INVALIDARG;
		};

		void updateList(const size_t actualPos, const int iteratedPos, const std::vector<VERTEX> container) {
			
			int counter{ -1 };
			for (size_t i = (actualPos - iteratedPos); i < actualPos; ++i) {
				++counter;
				this->vertexList[i].point[0] = container[counter].point[0];
				this->vertexList[i].point[1] = container[counter].point[1];
				this->vertexList[i].point[2] = container[counter].point[2];

				this->vertexList[i].normal[0] = container[counter].normal[0];
				this->vertexList[i].normal[1] = container[counter].normal[1];
				this->vertexList[i].normal[2] = container[counter].normal[2];

				this->vertexList[i].uv[0] = container[counter].uv[0];
				this->vertexList[i].uv[1] = container[counter].uv[1];
			}
		};
		void updateList(const size_t actualPos, const int iteratedPos, const std::vector<size_t> container)
		{
			int counter{ -1 };
			for (size_t i = (actualPos - iteratedPos); i < actualPos; ++i) {
				++counter;
				this->vertexIDs[i] = container[counter];
			}
		}
		
		void setShadingEngines(const std::vector<NODETYPES::Node*> shadingEngines)
		{
			this->shadingEngines = shadingEngines;
		}

	};
	
	class PointLight : public Node
	{
	private:
		float intensity {};
		float color[3] {};

	public:
		PointLight(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~PointLight() {};

		void setIntensity(const float intensity) { this->intensity = intensity; };
		void setColor(const float color[3]) { 
			this->color[0] = static_cast<float>(color[0]);
			this->color[1] = static_cast<float>(color[1]);
			this->color[2] = static_cast<float>(color[2]);
		};
	};

	class Camera : public Node
	{
	private:
		DirectX::XMMATRIX projectionMatrix{};

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
		};

	};

	class Texture : public Node
	{
	private:
		bool textureExist{};
		std::string filePath{};

		ComPtr<ID3D11ShaderResourceView>	gTextureSRV{ nullptr };
		ComPtr<ID3D11Resource>				gTexture{ nullptr };

	public:
		Texture(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Texture() {};

		HRESULT InitTexture(ID3D11Device* const gDevice, ID3D11DeviceContext* const gDeviceContext, const std::string filePath) {
				wchar_t fileName[200]{};
				for (size_t i = 0; i < filePath.length(); ++i)
				{
					fileName[i] = filePath[i];
				}
				return DirectX::CreateWICTextureFromFile(gDevice, fileName, &this->gTexture, &this->gTextureSRV, NULL);
			};

		bool getExistTexture() const { return this->textureExist; };
		std::string getFilePath() const { return this->filePath; };
		ID3D11ShaderResourceView* getTextureSRV() const { return this->gTextureSRV.Get(); };

		void toggleExistTexture(const bool flip) { this->textureExist = flip; };
		void setFilePath(const std::string& filePath) { this->filePath = filePath; };

	};

	class Bump : public Node
	{
	private:
		std::vector<NODETYPES::Node*> textures;
		std::vector<NODETYPES::Node*> shaders;

	public:
		Bump(const std::string& name, const std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Bump() {};

		void setTextures(const std::vector<NODETYPES::Node*> textures) { this->textures = textures; };
		void addShader(NODETYPES::Node* const shaderNode) { this->shaders.emplace_back(shaderNode); };

		std::vector<NODETYPES::Node*> getTextures() const { return this->textures; };
		NODETYPES::Texture* getTextureAt(const int index) const { return dynamic_cast<NODETYPES::Texture*>(this->textures[index]); };

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
	};
	////#############################################
	// 	#	Make support for master material node   #
	// 	#			( IF NECESSARY )				#
	// 	#############################################
#pragma region Material
	//class Material : public Node
	//{
	//private:
	//	bool diffuseConnected							{};
	//	DirectX::XMFLOAT3 color							{};
	//	std::vector<NODETYPES::Node*> diffuseMaps		{};

	//	bool colorMapConnected							{};
	//	DirectX::XMFLOAT3 ambientColor					{};
	//	std::vector<NODETYPES::Node*> colorMaps			{};

	//	bool transparancyMapConnected					{};
	//	DirectX::XMFLOAT3 transparancy					{};
	//	std::vector<NODETYPES::Node*> transparancyMaps	{};

	//public:
	//	Material() {};
	//	~Material() {};
	//	
	//};
#pragma endregion Material

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
			case ComLib::ATTRIBUTE_TYPE::NORMALMAP:
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
			case ComLib::ATTRIBUTE_TYPE::NORMALMAP:
				this->normalMaps = maps;
			}
		};
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

		bool specularMapConnected{};
		DirectX::XMFLOAT3 specular{};
		std::vector<NODETYPES::Node*> specularMaps{};

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
	};

	class ShadingEngine : public Node
	{
	private:
		std::vector<NODETYPES::Node*> materials{};

	public:
		ShadingEngine(std::string& name, std::string& uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~ShadingEngine() {};

		void setMaterials(std::vector<NODETYPES::Node*> materials) { this->materials = materials; };

		std::vector<NODETYPES::Node*> getMaterials() const { return this->materials; };
		NODETYPES::Node* getMaterialAt(const int index) const { return this->materials[index]; };

		size_t findMaterialIndex(const std::string materialUuid) {
			for (size_t i = 0; i < this->materials.size(); ++i){
				if (this->materials[i]->getUuid() == materialUuid) { return i; }
				else { return 0; }}};
		HRESULT findMaterial(std::string materialUuid, NODETYPES::Node* node) {
			for (size_t i = 0; i < materials.size(); ++i){
				if (this->materials[i]->getUuid() == materialUuid) { node = this->materials[i]; return S_OK;}
				else { return E_FAIL; }}};
	};
}