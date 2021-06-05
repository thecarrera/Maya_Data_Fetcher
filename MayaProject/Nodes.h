#pragma once
#include "Includes.h"

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
		Node* getParent(const int const index) const { 
			if (index <= parents.size() && index >= 0ULL) 
			{ return parents[index]; } 
			else 
			{ return nullptr; } 
		};

		void setName(const std::string& const name) { this->name = name; };
		void setUuid(const std::string& const uuid) { this->uuid = uuid; };
		void setType(const std::string  const type) { this->type = type; };
	
		void addParent(const Node* const parent) { this->parents.emplace_back(parent); };
		void removeParent(const std::string* parentUuid) {};
	};
	
	class Texture : public Node
	{
	private:
		bool textureExist {};
		std::string filePath {};

		ComPtr<ID3D11ShaderResourceView>	gTextureSRV	{ nullptr };
		ComPtr<ID3D11Resource>				gTexture	{ nullptr };

	public:
		Texture(std::string& const name, std::string& const uuid, const std::string const type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Texture() {};

		HRESULT InitTexture(ID3D11Device* gDevice, ID3D11DeviceContext* gDeviceContext, const std::string const filePath) {
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
		
		void toggleExistTexture(const bool const flip) { this->textureExist = flip; };
		void setFilePath(std::string& const filePath) { this->filePath = filePath; };

	};

	class Bump : public Node
	{
	private:
		std::vector<NODETYPES::Node*> textures;
		std::vector<NODETYPES::Node*> shaders;

	public:
		Bump(const std::string& const name, const std::string& const uuid, const std::string type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
		~Bump() {};

		void setTextures(const std::vector<NODETYPES::Node*> textures) { this->textures = textures; };
		void addShader(const NODETYPES::Node* shaderNode) { this->shaders.emplace_back(shaderNode); };

		std::vector<NODETYPES::Node*> getTextures() const { return this->textures; };
		NODETYPES::Texture* getTextureAt(const int index) const { return dynamic_cast<NODETYPES::Texture*>(this->textures[index]); };

		size_t findTextureIndex(const std::string textureUuid) {
			for (size_t i = 0; i < this->textures.size(); ++i) {
				if (this->textures[i]->getUuid() == textureUuid) { return i; }
			}};
		HRESULT findTexture(const std::string textureUuid, NODETYPES::Texture* texture) {
			for (size_t i = 0; i < this->textures.size(); ++i) {
				if (this->textures[i]->getUuid() == textureUuid) { texture = dynamic_cast<NODETYPES::Texture*>(this->textures[i]); }
			}};
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
		Lambert(std::string& const name, std::string& const uuid, const std::string const type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
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
		Blinn(std::string& const name, std::string& const uuid, const std::string const type) { this->setName(name); this->setUuid(uuid); this->setType(type); };
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
}