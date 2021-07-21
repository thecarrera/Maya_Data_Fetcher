#include "DirectX.h"

DX::DX()
{
	this->comlib = new ComLib("sharedFileMap", (25ULL << 23ULL)); //200MB
}
DX::~DX()
{
	delete comlib;
}
void DX::deallocateNodes()
{
	if (!this->transforms.empty())
	{
		this->transforms.clear();
	}
	if (!this->meshes.empty())
	{
		this->meshes.clear();
	}
	if (!this->pointLights.empty())
	{
		this->pointLights.clear();
	}
	if (!this->cameras.empty())
	{
		this->cameras.clear();
	}
	if (!this->textures.empty())
	{
		this->textures.clear();
	}
	if (!this->bumps.empty())
	{
		this->bumps.clear();
	}
	if (!this->lamberts.empty())
	{
		this->lamberts.clear();
	}
	if (!this->blinns.empty())
	{
		this->blinns.clear();
	}
	if (!this->shadingEngines.empty())
	{
		this->shadingEngines.clear();
	}
	if (!this->pureNodes.empty())
	{
		this->pureNodes.clear();
		this->activeCamera = nullptr;
	}
}
void DX::disconnect()
{
	if (SUCCEEDED(this->comlib->connectionStatus->sendPulse(Connection_Status::CONNECTION_TYPE::DISCONNECTED))) {}
	else {exit(-1);}
}

void DX::updatePointLight(char* msg)
{
	size_t messageOffset {};
	size_t uuidSize {};
	std::string uuid {};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node {this->findNode(uuid)};
	if (node)
	{
		NODETYPES::PointLight* pointLight { dynamic_cast<NODETYPES::PointLight*>(node) };
		if (pointLight)
		{
			size_t transformUuidSize {};
			std::string transformUuid {};
			float intensity {};
			float color[3]{};

			memcpy(&transformUuidSize, msg + messageOffset, STSIZE);
			messageOffset += STSIZE;
			transformUuid.resize(transformUuidSize);
			memcpy(&transformUuid[0], msg + messageOffset, transformUuidSize);
			messageOffset += transformUuidSize;

			NODETYPES::Node* transformer {this->findNode(transformUuid)};
			if (transformer)
			{
				pointLight->setTransform(transformer);
			}

			memcpy(&intensity, msg + messageOffset, sizeof(float));
			messageOffset += STSIZE;
			memcpy(&color, msg + messageOffset, sizeof(color));
			
			pointLight->setIntensity(intensity);
			pointLight->setColor(color);

		}
	}
}
void DX::updateMatrix(char* msg, ComLib::ATTRIBUTE_TYPE attr)
{
	size_t messageOffset{};
	size_t uuidSize{};
	std::string uuid{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node{ this->findNode(uuid) };
	if (node)
	{
		double matrix[4][4]{};
		memcpy(&matrix, msg + messageOffset, sizeof(double[4][4]));
		messageOffset += sizeof(double[4][4]);
		
		if (attr == ComLib::ATTRIBUTE_TYPE::MATRIX)
		{
			NODETYPES::Transform* transform {dynamic_cast<NODETYPES::Transform*>(node)};
			if (transform)
			{
				double worldMatrix[4][4] {};
				memcpy(&worldMatrix, msg + messageOffset, sizeof(double[4][4]));
				messageOffset += sizeof(double[4][4]);
				transform->setMatrix(matrix, 0);
				transform->setMatrix(worldMatrix, 1);
				transform->setupBuffers(this->gDevice.Get());

				if (node->getName() != "world")
				{
					size_t parentUuidSize {};
					std::string parentUuid {};
					memcpy(&parentUuidSize, msg + messageOffset, STSIZE);
					messageOffset += STSIZE;
					parentUuid.resize(parentUuidSize);
					memcpy(&parentUuid[0], msg + messageOffset, parentUuidSize);
					messageOffset += parentUuidSize;

					NODETYPES::Node* parentNode {this->findNode(parentUuid)};
					if (parentNode)
					{
						NODETYPES::Transform* parentTransform {dynamic_cast<NODETYPES::Transform*>(parentNode)};
						if (parentTransform)
						{
							transform->addParent(parentNode);
						}
					}
				}
			}
		}
		else if (attr == ComLib::ATTRIBUTE_TYPE::PROJMATRIX)
		{
			NODETYPES::Camera* camera{ dynamic_cast<NODETYPES::Camera*>(node) };
			if (camera)
			{
				camera->setProjectionMatrix(matrix);
				camera->setupBuffers(this->gDevice.Get());

				size_t viewUuidSize		{};
				std::string viewUuid	{};

				memcpy(&viewUuidSize, msg + messageOffset, STSIZE);
				messageOffset += STSIZE;
				viewUuid.resize(viewUuidSize);
				memcpy(&viewUuid[0], msg + messageOffset, viewUuidSize);
				messageOffset += viewUuidSize;

				NODETYPES::Node* viewMatrix {this->findNode(viewUuid)};
				if (viewMatrix)
				{
					camera->setViewMatrix(viewMatrix);
				}

			}
		}
	}
}
void DX::updateList(char* msg, ComLib::ATTRIBUTE_TYPE attribute)
{
	size_t messageOffset {};
	size_t uuidSize {};
	std::string uuid {};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node { this->findNode(uuid) };
	if (node)
	{
		NODETYPES::Mesh* mesh {dynamic_cast<NODETYPES::Mesh*>(node)};
		if (mesh)
		{
			if (attribute == ComLib::ATTRIBUTE_TYPE::VERTEX/* ||
				attribute == ComLib::ATTRIBUTE_TYPE::NORMAL*/)
			{
				UINT faceIndex   {};
				UINT actualPos   {};
				int iteratedPos {};

				memcpy(&faceIndex, msg + messageOffset, UINTSIZE);
				messageOffset += UINTSIZE;
				memcpy(&actualPos, msg + messageOffset, UINTSIZE);
				messageOffset += UINTSIZE;
				memcpy(&iteratedPos, msg + messageOffset, sizeof(int));
				messageOffset += sizeof(int);
				
				std::vector<NODETYPES::Mesh::VERTEX> container {};
				container.resize(100);
				UINT containerSize = sizeof(NODETYPES::Mesh::VERTEX) * static_cast<size_t>(iteratedPos + 1);
				memcpy(container.data(), msg + messageOffset, containerSize);
				messageOffset += containerSize;

				mesh->updateList(faceIndex, actualPos, iteratedPos, container);

				//###### OLD! #########
				//for (size_t i = 0; i < 10000; ++i)
				//{
				//	delete[] container[i];
				//}
				//delete[] container;
				//#####################
			}
			else if (attribute == ComLib::ATTRIBUTE_TYPE::VERTEXID)
			{
				UINT faceIndex {};
				UINT actualPos{};
				int iteratedPos{};

				memcpy(&faceIndex, msg + messageOffset, UINTSIZE);
				messageOffset += UINTSIZE;
				memcpy(&actualPos, msg + messageOffset, UINTSIZE);
				messageOffset += UINTSIZE;
				memcpy(&iteratedPos, msg + messageOffset, sizeof(int));
				messageOffset += sizeof(int);

				std::vector<UINT32> container {};
				container.resize(200);
				UINT containerSize {sizeof(UINT32) * static_cast<size_t>(iteratedPos + 1)};
				memcpy(&container[0], msg + messageOffset, containerSize);
				messageOffset += containerSize;

				mesh->updateList(faceIndex, actualPos, iteratedPos, container);
			}
			//###### OLD! #########
			//else if (attribute == ComLib::ATTRIBUTE_TYPE::UV)
			//{
			//	auto container { new double[10000][2] };
			//	memcpy(&container, msg + messageOffset, sizeof(container));
			//	messageOffset += sizeof(container);
			//
			//	size_t setIndex{};
			//	memcpy(&setIndex, msg + messageOffset, STSIZE);
			//	messageOffset += STSIZE;
			//
			//	mesh->updateList(actualPos, iteratedPos, setIndex, container);
			//
			//	for (size_t i = 0; i < 10000; ++i)
			//	{
			//		delete[] container[i];
			//	}
			//	delete[] container;
			//}
			//#####################
		}
	}
}
void DX::updateMeshTransform(char* msg)
{
	size_t messageOffset {};
	size_t uuidSize {};
	std::string uuid {};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node {this->findNode(uuid)};
	if (node)
	{
		NODETYPES::Mesh* mesh {dynamic_cast<NODETYPES::Mesh*>(node)};
		if (mesh)
		{
			size_t transformUuidSize	{};
			std::string transformUuid	{};
			memcpy(&transformUuidSize, msg + messageOffset, STSIZE);
			messageOffset += STSIZE;
			transformUuid.resize(transformUuidSize);
			memcpy(&transformUuid[0], msg + messageOffset, transformUuidSize);
			messageOffset += transformUuidSize;

			NODETYPES::Node* transformer {this->findNode(transformUuid)};
			if (transformer)
			{
				mesh->setTransform(transformer);
			}
		}
	}
}
void DX::updateMeshShaders(char* msg)
{
	size_t messageOffset		{};
	size_t uuidSize				{};
	std::string uuid			{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;
	
	NODETYPES::Node* node { this->findNode(uuid) };
	if (node)
	{
		NODETYPES::Mesh* mesh{ dynamic_cast<NODETYPES::Mesh*>(node) };
		if (mesh)
		{
			size_t shadingEngineCount {};
			memcpy(&shadingEngineCount, msg + messageOffset, STSIZE);
			messageOffset += STSIZE;

			std::vector<NODETYPES::Node*> shadingEngines{};

			for (size_t i = 0; i < shadingEngineCount; ++i)
			{
				size_t shadingEngineUuidSize {};
				std::string shadingEngineUuid{};
				memcpy(&shadingEngineUuidSize, msg + messageOffset, STSIZE);
				messageOffset += STSIZE;
				shadingEngineUuid.resize(shadingEngineUuidSize);
				memcpy(&shadingEngineUuid[0], msg + messageOffset, shadingEngineUuidSize);
				messageOffset += shadingEngineUuidSize;

				NODETYPES::Node* shaderEngine{ this->findNode(shadingEngineUuid) };
				if (shaderEngine)
				{
					shadingEngines.emplace_back(shaderEngine);
					shaderEngine->addNewReferenceBy(node, "shadingEngine");
				}

				mesh->setShadingEngines(shadingEngines);
			}
		}
	}
}
void DX::addShaderEngineMaterials(char* msg)
{
	size_t messageOffset{};
	size_t uuidSize{};
	std::string uuid{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node { this->findNode(uuid) };
	if (node)
	{
		NODETYPES::ShadingEngine* shadingEngine { dynamic_cast<NODETYPES::ShadingEngine*>(node) };
		if (shadingEngine)
		{
			size_t materialCount{};
			memcpy(&materialCount, msg + messageOffset, STSIZE);
			messageOffset += STSIZE;
			
			std::vector<NODETYPES::Node*> materials{};

			for (size_t i = 0; i < materialCount; ++i)
			{
				size_t materialUuidSize{};
				std::string materialUuid{};
				
				memcpy(&materialUuidSize, msg + messageOffset, STSIZE);
				messageOffset += STSIZE;
				materialUuid.resize(materialUuidSize);
				memcpy(&materialUuid[0], msg + messageOffset, materialUuidSize);
				messageOffset += materialUuidSize;

				NODETYPES::Node* materialNode{this->findNode(materialUuid)};
				if (materialNode)
				{
					materials.emplace_back(materialNode);
					materialNode->addNewReferenceBy(node,"shadingEngine");
				}
			}
			shadingEngine->setMaterials(materials);
		}
	}
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
	uuid.resize(uuidSize);
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
				NODETYPES::Lambert* lambert{ dynamic_cast<NODETYPES::Lambert*>(materialNode) };
				if (lambert)
				{
					lambert->setTextureConnected(textureConnected, attribute);

					memcpy(&connectedTextureCount, msg + messageOffset, STSIZE);
					messageOffset += STSIZE;

					std::vector<NODETYPES::Node*> textureUuids;
					textureUuids.resize(connectedTextureCount);

					size_t textureUuidSize{};
					std::string textureUuid{};
					for (size_t i = 0; i < connectedTextureCount; ++i)
					{
						memcpy(&textureUuidSize, msg + messageOffset, STSIZE);
						messageOffset += STSIZE;
						textureUuid.resize(textureUuidSize);
						memcpy(&textureUuid[0], msg + messageOffset, textureUuidSize);
						messageOffset += textureUuidSize;

						NODETYPES::Node* textureNode{ this->findNode(textureUuid) };
						if (textureNode)
						{
							textureUuids.emplace_back(textureNode);

							switch (attribute)
							{
							case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
								textureNode->addNewReferenceBy(materialNode, "DiffuseMap");
								break;
							case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
								textureNode->addNewReferenceBy(materialNode, "ColorMap");
								break;
							case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
								textureNode->addNewReferenceBy(materialNode, "TransparencyMap");
								break;
							case ComLib::ATTRIBUTE_TYPE::SHNORMALMAP:
								//node is not a texture but a bump2d
								textureNode->addNewReferenceBy(materialNode, "NormalMap");
								break;
							//case ComLib::ATTRIBUTE_TYPE::SHSPECULARMAP:
							}
						}
					}
					lambert->setTextureMaps(textureUuids, attribute);
				}
			}
			else if (materialNode->getType() == "blinn")
			{
				NODETYPES::Blinn* blinn{ dynamic_cast<NODETYPES::Blinn*>(materialNode) };
				if (blinn)
				{
					std::vector<NODETYPES::Node*> textureUuids;
					textureUuids.resize(connectedTextureCount);
					blinn->setTextureConnected(textureConnected, attribute);

					memcpy(&connectedTextureCount, msg + messageOffset, STSIZE);
					messageOffset += STSIZE;

					size_t textureUuidSize{};
					std::string textureUuid{};
					for (size_t i = 0; i < connectedTextureCount; ++i)
					{
						memcpy(&textureUuidSize, msg + messageOffset, STSIZE);
						messageOffset += STSIZE;
						textureUuid.resize(textureUuidSize);
						memcpy(&textureUuid[0], msg + messageOffset, textureUuidSize);
						messageOffset += textureUuidSize;

						NODETYPES::Node* textureNode{ this->findNode(textureUuid) };
						if (textureNode)
						{
							textureUuids.emplace_back(textureNode);

							switch (attribute)
							{
							case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
								textureNode->addNewReferenceBy(materialNode, "DiffuseMap");
								break;
							case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
								textureNode->addNewReferenceBy(materialNode, "ColorMap");
								break;
							case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
								textureNode->addNewReferenceBy(materialNode, "TransparencyMap");
								break;
							case ComLib::ATTRIBUTE_TYPE::SHNORMALMAP:
								textureNode->addNewReferenceBy(materialNode, "NormalMap");
								break;
							}
						}
					}
					blinn->setTextureMaps(textureUuids, attribute);
				}
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
	uuid.resize(uuidSize);
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
	uuid.resize(uuidSize);
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
			shaderUuid.resize(shaderUuidSize);
			memcpy(&shaderUuid[0], msg + messageOffset, shaderUuidSize);
			messageOffset += shaderUuidSize;	

			NODETYPES::Node* shaderNode{this->findNode(shaderUuid)};
			if (shaderNode)
			{
				bump->addNewReferenceBy(node, "bump");
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
	uuid.resize(uuidSize);
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
			textureUuids.resize(textureCount);
			for (size_t i = 0; i < textureCount; ++i)
			{
				size_t textureUuidSize{};
				std::string textureUuid{};

				memcpy(&textureUuidSize, msg + messageOffset, STSIZE);
				messageOffset += STSIZE;
				textureUuid.resize(textureUuidSize);
				memcpy(&textureUuid[0], msg + messageOffset, textureUuidSize);
				messageOffset += textureUuidSize;

				NODETYPES::Node* textureNode{ this->findNode(textureUuid) };
				if (textureNode)
				{
					NODETYPES::Texture* texture{ dynamic_cast<NODETYPES::Texture*>(textureNode) };
					if (texture)
					{
						textureUuids.emplace_back(this->findNode(textureUuid));
						texture->addNewReferenceBy(node, "BumpTex");
					}
				}
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
	uuid.resize(uuidSize);
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
			path.resize(pathSize);
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
	size_t messageOffset {};
	size_t targetUuidSize {};
	std::string targetUuid {};
	memcpy(&targetUuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	targetUuid.resize(targetUuidSize);
	memcpy(&targetUuid[0], msg + messageOffset, targetUuidSize);
	messageOffset += targetUuidSize;
	
	NODETYPES::Node* node {this->findNode(targetUuid)};
	if (node)
	{
		std::string type {node->getType()};
		size_t parentUuidSize {};
		std::string parentUuid {};

		memcpy(&parentUuidSize, msg + messageOffset, STSIZE);
		messageOffset += STSIZE;
		parentUuid.resize(parentUuidSize);
		memcpy(&parentUuid[0], msg + messageOffset, parentUuidSize);
		messageOffset += parentUuidSize;

		NODETYPES::Node* parentNode {this->findNode(parentUuid)};
		if (parentNode)
		{
			if (type == "transform")
			{
				NODETYPES::Transform* transform {dynamic_cast<NODETYPES::Transform*>(node)};
				transform->addParent(parentNode);
			}
			else if (type == "mesh")
			{
				NODETYPES::Mesh* mesh {dynamic_cast<NODETYPES::Mesh*>(node)};
				mesh->setTransform(parentNode);
			}
			else if (type == "pointLight")
			{
				NODETYPES::PointLight* pointLight {dynamic_cast<NODETYPES::PointLight*>(node)};
				pointLight->setTransform(parentNode);
			}
			else if (type == "camera")
			{
				NODETYPES::Camera* camera {dynamic_cast<NODETYPES::Camera*>(node)};
				camera->setViewMatrix(parentNode);
			}
		}
	}
}
void DX::removeParent(char* msg)
{
	size_t messageOffset {};
	size_t targetUuidSize {};
	std::string targetUuid {};
	memcpy(&targetUuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	targetUuid.resize(targetUuidSize);
	memcpy(&targetUuid[0], msg + messageOffset, targetUuidSize);
	messageOffset += targetUuidSize;

	NODETYPES::Node* node{ this->findNode(targetUuid) };
	if (node)
	{
		std::string type{ node->getType() };
		size_t parentUuidSize{};
		std::string parentUuid{};

		memcpy(&parentUuidSize, msg + messageOffset, STSIZE);
		messageOffset += STSIZE;
		parentUuid.resize(parentUuidSize);
		memcpy(&parentUuid[0], msg + messageOffset, parentUuidSize);
		messageOffset += parentUuidSize;

		NODETYPES::Node* parentNode{ this->findNode(parentUuid) };
		if (parentNode)
		{
			if (type == "transform")
			{
				NODETYPES::Transform* transform{ dynamic_cast<NODETYPES::Transform*>(node) };
				transform->removeParent(parentNode);
			}
			else if (type == "mesh")
			{
				NODETYPES::Mesh* mesh{ dynamic_cast<NODETYPES::Mesh*>(node) };
				mesh->removeTransformReference();
			}
			else if (type == "pointLight")
			{
				NODETYPES::PointLight* pointLight{ dynamic_cast<NODETYPES::PointLight*>(node) };
				pointLight->removeTransformReference();
			}
			else if (type == "camera")
			{
				NODETYPES::Camera* camera{ dynamic_cast<NODETYPES::Camera*>(node) };
				camera->removeViewMatrixReference();
			}
		}
	}
}
void DX::setActiveCamera(char* msg) {
	size_t messageOffset {};
	std::string uuid {};
	size_t uuidSize{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node {this->findNode(uuid)};
	if (node)
	{
		NODETYPES::Camera* camera {dynamic_cast<NODETYPES::Camera*>(node)};
		if (camera)
		{
			this->activeCamera = camera;
		}
	}
}
void DX::allocateNode(char* msg)
{
	size_t messageOffset {};
	
	size_t typeSize{};
	std::string type{};
	size_t nameSize{};
	std::string name{};
	size_t uuidSize{};
	std::string uuid{};

	memcpy(&typeSize, msg, STSIZE);
	messageOffset += STSIZE;
	type.resize(typeSize);
	memcpy(&type[0], msg + messageOffset, typeSize);
	messageOffset += typeSize;

	memcpy(&nameSize, msg + messageOffset, STSIZE);
	messageOffset += STSIZE;
	name.resize(nameSize);
	memcpy(&name[0], msg + messageOffset, nameSize);
	messageOffset += nameSize;
	
	memcpy(&uuidSize, msg + messageOffset, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	if (type == "transform") // NOTE: Change the rest
	{
		NODETYPES::Transform* transform{ new NODETYPES::Transform{name, uuid, "transform"}};
		this->pureNodes.emplace_back(transform, uuid);
		// Check if buffer needs extending
		this->transforms.emplace_back(transform);
	}
	else if (type == "mesh")
	{
		NODETYPES::Mesh* mesh{ new NODETYPES::Mesh{name, uuid, "mesh"}};
		this->pureNodes.emplace_back(mesh, uuid);
		this->meshes.emplace_back(mesh);
	}
	else if (type == "pointLight")
	{
		NODETYPES::PointLight* pointLight{ new NODETYPES::PointLight{name, uuid, "mesh"}};
		this->pureNodes.emplace_back(pointLight, uuid);
		this->pointLights.emplace_back(pointLight);
	}
	else if (type == "camera")
	{
		NODETYPES::Camera* camera{ new NODETYPES::Camera{name, uuid, "mesh"}};
		this->pureNodes.emplace_back(camera, uuid);
		this->cameras.emplace_back(camera);
	}
	else if (type == "shadingEngine")
	{
		NODETYPES::ShadingEngine* shadingEngine{ new NODETYPES::ShadingEngine{name, uuid, "shadingEngine"}};
		this->pureNodes.emplace_back(shadingEngine, uuid);
		this->shadingEngines.emplace_back(shadingEngine);
	}
	else if (type == "blinn")
	{
		NODETYPES::Blinn* blinn{ new NODETYPES::Blinn{name, uuid, "blinn"}};
		this->pureNodes.emplace_back(blinn, uuid);
		this->blinns.emplace_back(blinn);
		
	}
	else if (type == "lambert")
	{
		NODETYPES::Lambert* lambert{ new NODETYPES::Lambert{name, uuid, "lambert"}};
		this->pureNodes.emplace_back(lambert, uuid);
		this->lamberts.emplace_back(lambert);
	}
	else if (type == "file")
	{
		NODETYPES::Texture* tex{ new NODETYPES::Texture{name, uuid, "texture"}};
		this->pureNodes.emplace_back(tex, uuid);
		this->textures.emplace_back(tex);
	}
	else if (type == "bump2d")
	{
		NODETYPES::Bump* bump{ new NODETYPES::Bump{name, uuid, "bump"}};
		this->pureNodes.emplace_back(bump, uuid);
		this->bumps.emplace_back(bump);
	}
	else if (type == "dagNode")
	{
		if (name == "world")
		{
			NODETYPES::Transform* transform{ new NODETYPES::Transform{name, uuid, "world"} };
			this->pureNodes.emplace_back(transform, uuid);
			this->transforms.emplace_back(transform);
		}
	}

}
void DX::deallocateNode(char* msg) {
	size_t messageOffset {};
	size_t uuidSize {};
	std::string uuid {};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node {this->findNode(uuid)};
	if (node)
	{
		node->removeReferences();
		
		if (node->getType() == "transform") {
			NODETYPES::Transform* transform {dynamic_cast<NODETYPES::Transform*>(node)};
			transform->clearParents();
			transform->clearChildren();
		}
		else if (node->getType() == "mesh") {
			NODETYPES::Mesh* mesh {dynamic_cast<NODETYPES::Mesh*>(node)};
			mesh->clearShadingEngines();
		}
		else if (node->getType() == "pointLight") {
			NODETYPES::PointLight* pointLight {dynamic_cast<NODETYPES::PointLight*>(node)};
			pointLight->clearTransformReference();
		}
		else if (node->getType() == "camera") {
			NODETYPES::Camera* camera {dynamic_cast<NODETYPES::Camera*>(node)};
			camera->clearViewMatrixReference();
		}
		else if (node->getType() == "shadingEngine") {
			NODETYPES::ShadingEngine* shadingEngine {dynamic_cast<NODETYPES::ShadingEngine*>(node)};
			shadingEngine->clearMaterials();
		}
		else if (node->getType() == "blinn") {
			NODETYPES::Blinn* blinn {dynamic_cast<NODETYPES::Blinn*>(node)};
			blinn->clearDiffuseMaps();
			blinn->clearColorMaps();
			blinn->clearTransparancyMaps();
			blinn->clearNormalMaps();
		}
		else if (node->getType() == "lambert") {
			NODETYPES::Lambert* lambert {dynamic_cast<NODETYPES::Lambert*>(node)};
			lambert->clearDiffuseMaps();
			lambert->clearColorMaps();
			lambert->clearTransparancyMaps();
			lambert->clearNormalMaps();
		}
		else if (node->getType() == "bump2d") {
			NODETYPES::Bump* bump {dynamic_cast<NODETYPES::Bump*>(node)};
			bump->clearTextures();
		}
	}
}
void DX::allocateVertex(char* msg)
{
	size_t messageOffset{};
	size_t uuidSize{};
	std::string uuid{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node {this->findNode(uuid)};
	if (node)
	{
		NODETYPES::Mesh* mesh {dynamic_cast<NODETYPES::Mesh*>(node)};
		if (mesh)
		{
			UINT faceIndex {};
			UINT vertexCount {};
			UINT vertexIDCount {};
			memcpy(&faceIndex, msg + messageOffset, UINTSIZE);
			messageOffset += UINTSIZE;
			memcpy(&vertexCount, msg + messageOffset, UINTSIZE);
			messageOffset += UINTSIZE;
			memcpy(&vertexIDCount, msg + messageOffset, UINTSIZE);
			messageOffset += UINTSIZE;

			mesh->allocateVertices(
				this->gDevice.Get(),
				faceIndex,
				vertexCount,
				vertexIDCount
			);
		}
	}
}
void DX::allocateFaces(char* msg)
{
	size_t messageOffset	{};
	size_t uuidSize			{};
	std::string uuid		{};

	memcpy(&uuidSize, msg, STSIZE);
	messageOffset += STSIZE;
	uuid.resize(uuidSize);
	memcpy(&uuid[0], msg + messageOffset, uuidSize);
	messageOffset += uuidSize;

	NODETYPES::Node* node {this->findNode(uuid)};
	if (node)
	{
		NODETYPES::Mesh* mesh {dynamic_cast<NODETYPES::Mesh*>(node)};
		if (mesh)
		{
			UINT faceCount {};
			memcpy(&faceCount, msg + messageOffset, UINTSIZE);
			messageOffset += UINTSIZE;

			if (faceCount)
			{
				mesh->allocateFaces(faceCount);
			}
		}
	}
}
HRESULT DX::loadingObjects()
{
	if (this->comlib->hFileMap)
	{	
		char* msg {this->comlib->recv()};
		if (msg)
		{
			char* data {msg + sizeof(ComLib::HEADER)};
			ComLib::HEADER* header {reinterpret_cast<ComLib::HEADER*>(msg)};
			switch (header->msgId)
			{
			case ComLib::MSG_TYPE::ACTIVECAM:
				this->setActiveCamera(data);
				break;
			case ComLib::MSG_TYPE::ALLOCATE:
				switch (header->attrID) {
				case ComLib::ATTRIBUTE_TYPE::NODE:
					this->allocateNode(data);
					break;
				case ComLib::ATTRIBUTE_TYPE::VERTEX:
				//case ComLib::ATTRIBUTE_TYPE::NORMAL:
				//case ComLib::ATTRIBUTE_TYPE::UVSETS:
				//case ComLib::ATTRIBUTE_TYPE::UVSET:
				//case ComLib::ATTRIBUTE_TYPE::UVID:
					this->allocateVertex(data);
					break;
				case ComLib::ATTRIBUTE_TYPE::FACES:
					this->allocateFaces(data);
					break;
				}
				break;
			case ComLib::MSG_TYPE::DEALLOCATE:
				switch (header->attrID) {
				case ComLib::ATTRIBUTE_TYPE::NODE:
					this->deallocateNode(data);
					break;
				}
				break;
			case ComLib::MSG_TYPE::ADDVALUES:
				switch (header->attrID) {
				case ComLib::ATTRIBUTE_TYPE::PARENT:
					this->addParent(data);
					break;
				}
				break;
			case ComLib::MSG_TYPE::UPDATEVALUES:
				switch (header->attrID) {
				case ComLib::ATTRIBUTE_TYPE::MATRIX:
					this->updateMatrix(data, header->attrID);
					break;
				case ComLib::ATTRIBUTE_TYPE::VERTEX:
				case ComLib::ATTRIBUTE_TYPE::VERTEXID:
				//case ComLib::ATTRIBUTE_TYPE::NORMAL:
				//case ComLib::ATTRIBUTE_TYPE::UV:
					this->updateList(data, header->attrID);
					break;
				case ComLib::ATTRIBUTE_TYPE::MESHTRANSFORMER:
					this->updateMeshTransform(data);
					break;
				case ComLib::ATTRIBUTE_TYPE::MESHSHADERS:
					this->updateMeshShaders(data);
					break;
				case ComLib::ATTRIBUTE_TYPE::PROJMATRIX:
					this->updateMatrix(data, header->attrID);
					break;
				case ComLib::ATTRIBUTE_TYPE::POINTINTENSITY:
					this->updatePointLight(data);
					break;
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
					this->addMaterialChannels(data, header->attrID);
					break;
				case ComLib::ATTRIBUTE_TYPE::SHCOLORMAP:
				case ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP:
				case ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP:
				case ComLib::ATTRIBUTE_TYPE::SHNORMALMAP:
					this->addMaterialTextures(data, header->attrID);
					break;
				case ComLib::ATTRIBUTE_TYPE::SESURFACE:
					this->addShaderEngineMaterials(data);
					break;
				}
				break;
			case ComLib::MSG_TYPE::REMOVEVALUES:
				switch (header->attrID) {
				case ComLib::ATTRIBUTE_TYPE::PARENT:
					this->removeParent(data);
					break;
				default:
					break;
				}
				break;
			case ComLib::MSG_TYPE::MESSAGE:
				switch (header->attrID) {
				case ComLib::ATTRIBUTE_TYPE::OFFEND:
				case ComLib::ATTRIBUTE_TYPE::ATTREND:
					this->comlib->mutex.Lock();
					// Fetching size_t messageCount
					size_t* messageCount {this->comlib->mSize - 1};
					*messageCount -= 1;
					this->comlib->mutex.Unlock();
					return S_OK;
				}
				break;
			}
		}
	}
	else
	{
		this->deallocateNodes();
		return S_OK;
	}
	return E_FAIL;
}
HRESULT DX::queryExistingData()
{
	if (comlib->peekExistingMessage())
	{
		HRESULT fetching {E_FAIL};
		char* msg {this->comlib->recv()};
		if (msg)
		{
			ComLib::HEADER* header{ reinterpret_cast<ComLib::HEADER*>(msg) };

			switch (header->msgId)
			{
			case ComLib::MSG_TYPE::MESSAGE:
				switch (header->attrID)
				{
				case ComLib::ATTRIBUTE_TYPE::OFFSTART:
					while (FAILED(fetching))
					{
						fetching = loadingObjects();
					}
					return S_OK;
					break;
				}
				break;
			}
		}
	}
	else
	{
		return E_FAIL;
	}
	return E_FAIL;
}

void DX::createDefaultTextures()
{
	HRESULT hr {};
	D3D11_TEXTURE2D_DESC defaultTextureDesc{
		.Width			{1},
		.Height			{1},
		.MipLevels		{1},
		.ArraySize		{1},
		.Format			{DXGI_FORMAT_R8G8B8A8_UNORM},
		.Usage			{D3D11_USAGE_IMMUTABLE},
		.BindFlags		{D3D11_BIND_SHADER_RESOURCE},
	};
	defaultTextureDesc.SampleDesc.Count = 1;
	static const uint32_t color = 0x8E8E8E;
	D3D11_SUBRESOURCE_DATA dataPtr{ &color, sizeof(uint32_t), 0 };
	hr = this->gDevice->CreateTexture2D(&defaultTextureDesc, &dataPtr, &this->defaultTextureMap);
	if (FAILED(hr)) { exit(-1); }
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{
		.Format {DXGI_FORMAT_R8G8B8A8_UNORM},
		.ViewDimension {D3D11_SRV_DIMENSION_TEXTURE2D},
	};
	SRVDesc.Texture2D.MipLevels = 1;
	hr = this->gDevice->CreateShaderResourceView(this->defaultTextureMap.Get(), &SRVDesc, &this->defaultTextureSRV);
	if (FAILED(hr)) { exit(-1); }
}
void DX::CreateSamplerStates()
{
	HRESULT hr {};

	//Diffuse Sampler State
	D3D11_SAMPLER_DESC texSampDesc {
	.Filter {D3D11_FILTER_MIN_MAG_MIP_POINT},
	.AddressU {D3D11_TEXTURE_ADDRESS_CLAMP},
	.AddressV {D3D11_TEXTURE_ADDRESS_CLAMP},
	.AddressW {D3D11_TEXTURE_ADDRESS_CLAMP},
	.MipLODBias {},
	.MaxAnisotropy {PIXELSAMPLE},
	.ComparisonFunc {D3D11_COMPARISON_NEVER},
	.MinLOD {},
	.MaxLOD {}
	};

	hr = this->gDevice->CreateSamplerState(&texSampDesc, &this->txSamplerState);
	if (FAILED(hr)){
		exit(-1);
	}
}
void DX::CreateBuffers()
{
	HRESULT hr {};
	
	//#############################################################
	//#						Camera buffer 						  #
	//#############################################################
	D3D11_BUFFER_DESC cameraBufferDesc{
		.ByteWidth {sizeof(DirectX::XMMATRIX)},
		.Usage {D3D11_USAGE_DYNAMIC},
		.BindFlags {D3D11_BIND_CONSTANT_BUFFER},
		.CPUAccessFlags {D3D11_CPU_ACCESS_WRITE},
		.MiscFlags {},
		.StructureByteStride {}
	};

	hr = gDevice->CreateBuffer(&cameraBufferDesc, nullptr, &this->cameraBuffer[0]);
	if (FAILED(hr)) { exit(-1); }

	hr = gDevice->CreateBuffer(&cameraBufferDesc, nullptr, &this->cameraBuffer[1]);
	if (FAILED(hr)) { exit(-1); }

	//#############################################################
	//#						transform buffers					  #
	//#############################################################
	// OBJECT WORLD MAT
	D3D11_BUFFER_DESC transformBufferDesc{
		.ByteWidth {sizeof(DirectX::XMMATRIX)},
		.Usage {D3D11_USAGE_DYNAMIC},
		.BindFlags {D3D11_BIND_CONSTANT_BUFFER},
		.CPUAccessFlags {D3D11_CPU_ACCESS_WRITE},
		.MiscFlags {},
		.StructureByteStride {}
	};
	hr = gDevice->CreateBuffer(&transformBufferDesc, nullptr, &this->transformBuffer);
	if (FAILED(hr)) { exit(-1);}

	//#############################################################
	//#						PointLight buffers					  #
	//#############################################################

	D3D11_BUFFER_DESC pointDataBufferDesc{
		.ByteWidth {sizeof(ACTIVEPOINTLIGHTS)},
		.Usage {D3D11_USAGE_DYNAMIC},
		.BindFlags {D3D11_BIND_CONSTANT_BUFFER},
		.CPUAccessFlags {D3D11_CPU_ACCESS_WRITE},
		.MiscFlags {},
		.StructureByteStride {}
	};
	hr = gDevice->CreateBuffer(&pointDataBufferDesc, nullptr, &this->pointLightDataBuffer);
	if (FAILED(hr)) { exit(-1); }
}

HRESULT DX::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT arraySize, ID3DBlob* VS)
{
	return this->gDevice->CreateInputLayout(inputDesc, arraySize, VS->GetBufferPointer(), VS->GetBufferSize(), &this->gVertexLayout);
}
HRESULT DX::CreateVertexShader(LPCWSTR fileName, LPCSTR entryPoint, D3D11_INPUT_ELEMENT_DESC inputDesc[], UINT arraySize, ID3D11VertexShader*& vertexShader)
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
		hr = this->gDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &vertexShader);
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
HRESULT DX::CreateGeometryShader(LPCWSTR fileName, LPCSTR entryPoint, ID3D11GeometryShader*& geometricShader)
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
		return this->gDevice->CreateGeometryShader(GS->GetBufferPointer(), GS->GetBufferSize(), nullptr, &geometricShader);
	}
	else
	{
		OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
		return 0x80004005;//E_FAIL
	}
}
HRESULT DX::CreatePixelShader(LPCWSTR fileName, LPCSTR entryPoint, ID3D11PixelShader*& pixelShader)
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
		return this->gDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &pixelShader);
	}
	else
	{
		OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
		return 0x80004005;//E_FAIL
	}
}
void DX::CreateShaders()
{	
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	
	if (FAILED(this->CreateVertexShader(L"G-Buffer_VertexShader.hlsl", "VS_main", inputDesc, ARRAYSIZE(inputDesc), *this->gGBufferVertexShader.GetAddressOf())))
	{
		OutputDebugStringA("Failed to setup vertex shader!");
		exit(-1);
	}
	/*
	if (FAILED(this->CreateGeometryShader(L"G-Buffer_GeometryShader.hlsl", "GS_main", *this->gGBufferGeometryShader.GetAddressOf())))
	{
		OutputDebugStringA("Failed to create geometry shader!");
		exit(-1);
	}*/

	if (FAILED(this->CreatePixelShader(L"G-Buffer_PixelShader.hlsl", "PS_main", *this->gGBufferPixelShader.GetAddressOf())))
	{
		OutputDebugStringA("Failed to create pixel shader");
		exit(-1);
	}

	if (FAILED(this->CreatePixelShader(L"LightPass_PixelShader.hlsl", "PS_main", *this->gLightPassShader.GetAddressOf())))
	{
		OutputDebugStringA("Failed to create pixel shader");
		exit(-1);
	}
}

void DX::setScissorRect()
{
	D3D11_RECT rects[1];
	rects[0].left = 0;
	rects[0].right = WIDTH;
	rects[0].top = 0;
	rects[0].bottom = HEIGHT;

	this->gDeviceContext->RSSetScissorRects(1, rects);
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
	DXGI_MODE_DESC swapModeDesc{
		.Width {static_cast<UINT>(WIDTH)},
		.Height {static_cast<UINT>(HEIGHT)},
		.Format {DXGI_FORMAT_R8G8B8A8_UNORM}
	};
	DXGI_SAMPLE_DESC sampleDesc {
		.Count {PIXELSAMPLE}
	};
	DXGI_SWAP_CHAIN_DESC swapDesc{
		.BufferDesc {swapModeDesc},
		.SampleDesc {sampleDesc},
		.BufferUsage {DXGI_USAGE_RENDER_TARGET_OUTPUT},
		.BufferCount {2},
		.OutputWindow {*wndHandle},
		.Windowed {TRUE},
		.SwapEffect {DXGI_SWAP_EFFECT_FLIP_DISCARD}	
	};

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
	DXGI_SAMPLE_DESC sampleDesc {
		.Count {PIXELSAMPLE},
	};
	D3D11_TEXTURE2D_DESC depthBufferDesc {
		.Width {static_cast<UINT>(WIDTH)},
		.Height {static_cast<UINT>(HEIGHT)},
		.MipLevels {1},
		.ArraySize {1},
		.Format {DXGI_FORMAT_D24_UNORM_S8_UINT},
		.SampleDesc {sampleDesc},
		.Usage {D3D11_USAGE_DEFAULT},
		.BindFlags {D3D11_BIND_DEPTH_STENCIL},
	};

	return this->gDevice->CreateTexture2D(&depthBufferDesc, nullptr, &this->gDepthStencil[index]);
}
HRESULT DX::CreateDepthStencil(int index, ID3D11Texture2D* pBackBuffer)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV {
		.Format {DXGI_FORMAT_D24_UNORM_S8_UINT},
		.ViewDimension {D3D11_DSV_DIMENSION_TEXTURE2D}
	};

	return this->gDevice->CreateDepthStencilView(this->gDepthStencil[index].Get(), &descDSV, &this->gDepthStencilView[index]);
}
HRESULT DX::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCILOP_DESC dsFrontFaceOPDesc {
		.StencilFailOp {D3D11_STENCIL_OP_KEEP},
		.StencilDepthFailOp {D3D11_STENCIL_OP_INCR},
		.StencilPassOp {D3D11_STENCIL_OP_KEEP},
		.StencilFunc {D3D11_COMPARISON_ALWAYS}
	};
	D3D11_DEPTH_STENCILOP_DESC dsBackFaceOPDesc{
		.StencilFailOp {D3D11_STENCIL_OP_KEEP},
		.StencilDepthFailOp {D3D11_STENCIL_OP_DECR},
		.StencilPassOp {D3D11_STENCIL_OP_KEEP},
		.StencilFunc {D3D11_COMPARISON_ALWAYS}
	};
	D3D11_DEPTH_STENCIL_DESC dsStateDesc {
		.DepthEnable {true},
		.DepthWriteMask {D3D11_DEPTH_WRITE_MASK_ALL},
		.DepthFunc {D3D11_COMPARISON_LESS},
		.StencilEnable {true},
		.StencilReadMask {0xFF}, // 0xFF == 250 + 5 (255)
		.StencilWriteMask {0xFF},
		.FrontFace {dsFrontFaceOPDesc},
		.BackFace {dsBackFaceOPDesc}
	};

	return this->gDevice->CreateDepthStencilState(&dsStateDesc, this->gDepthStencilState.GetAddressOf());
}
HRESULT DX::CreateGBufferResources()
{
	HRESULT hr{};
	D3D11_TEXTURE2D_DESC renderTextureDesc{
		.Width			{(UINT)WIDTH},
		.Height			{(UINT)HEIGHT},
		.MipLevels		{1},
		.ArraySize		{1},
		.Format			{DXGI_FORMAT_R32G32B32A32_FLOAT},
		.Usage			{D3D11_USAGE_DEFAULT},
		.BindFlags		{D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE},
	};
	renderTextureDesc.SampleDesc.Count = 1;
	for (size_t i = 0; i < 3; ++i)
	{
		hr = this->gDevice->CreateTexture2D(&renderTextureDesc, nullptr, &this->gGBufferTextures[i]);
		if (FAILED(hr)) { return hr; }
	}

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{
		.Format {DXGI_FORMAT_R32G32B32A32_FLOAT},
		.ViewDimension {D3D11_RTV_DIMENSION_TEXTURE2D}
	};
	for (size_t i = 0; i < 3; ++i)
	{
		hr = this->gDevice->CreateRenderTargetView(this->gGBufferTextures[i].Get(), &RTVDesc, &this->gGBufferRTVs[i]);
		if (FAILED(hr)) { return hr; }
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc {
		.Format {DXGI_FORMAT_R32G32B32A32_FLOAT},
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D
	};
	SRVDesc.Texture2D.MipLevels = 1;
	for (size_t i = 0; i < 3; ++i)
	{
		hr = this->gDevice->CreateShaderResourceView(this->gGBufferTextures[i].Get(), &SRVDesc, &this->gGBufferSRVs[i]);
		if (FAILED(hr)) { return hr; }
	}

	return S_OK;
}
HRESULT DX::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasDesc{
		.FillMode {D3D11_FILL_SOLID},
		.CullMode = {D3D11_CULL_NONE},
		.FrontCounterClockwise {},
		.DepthBias {},
		.DepthBiasClamp {},
		.SlopeScaledDepthBias {},
		.DepthClipEnable {true},
		.ScissorEnable {true},
		.MultisampleEnable {},
		.AntialiasedLineEnable {}
	};
	return this->gDevice->CreateRasterizerState(&rasDesc, this->gRasterizerState.GetAddressOf());
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
		
		//if (FAILED(this->CreateDepthStencilState()))
		//{
		//	OutputDebugStringA("Failed to create depth stencil state!");
		//	exit(-1);
		//}

		if (FAILED(this->CreateGBufferResources()))
		{
			OutputDebugStringA("Failed to create G-Buffer resources!");
			exit(-1);
		}

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
	this->setScissorRect();
	this->CreateShaders();
	this->CreateBuffers();
	this->CreateSamplerStates();
	this->createDefaultTextures();

	//Set-up ComLib and fetch data
	if (SUCCEEDED(this->comlib->connectionStatus->sendPulse(Connection_Status::CONNECTION_TYPE::CONNECTED)))
	{
		HRESULT fetching {E_FAIL};
		while (FAILED(fetching)) {
			fetching = this->queryExistingData();
		}
		this->Render();
	}
	else {
		exit(-1);
	}
}

void DX::Update()
{
	if (this->comlib->hFileMap)
	{
		//Might retrieve info late if frame 
		if (comlib->peekExistingMessage())
		{
			char* msg{ this->comlib->recv() };
			if (msg)
			{
				HRESULT fetching{ E_FAIL };
				ComLib::HEADER* header{ reinterpret_cast<ComLib::HEADER*>(msg) };

 				switch (header->msgId)
				{
				case ComLib::MSG_TYPE::MESSAGE:
					switch (header->attrID)
					{
					case ComLib::ATTRIBUTE_TYPE::OFFSTART:
					case ComLib::ATTRIBUTE_TYPE::ATTRST:
						while (FAILED(fetching))
						{
							fetching = loadingObjects();
						}
						this->Render();
						break;
					}
					break;
				default:
					break;
				}								
			}
		}
	}
}

void DX::updateVertexBuffers(UINT meshIndex, UINT faceIndex) {
	this->meshes[meshIndex].get()->updateVertexListToBuffer(faceIndex, this->gDeviceContext.Get());
	this->meshes[meshIndex].get()->updateVertexIDToBuffer(faceIndex, this->gDeviceContext.Get());
}
void DX::UpdatePointLightBuffers() 
{
	if (this->pointLights.size() <= 20)
		this->activePointLights.pointLightCount = static_cast<UINT32>(this->pointLights.size());
	else
		this->activePointLights.pointLightCount = 20;

	D3D11_MAPPED_SUBRESOURCE dataPtr {};
	this->gDeviceContext->Map(this->pointLightDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	for (size_t i = 0; i < this->activePointLights.pointLightCount; ++i)
	{
		this->pointLights[i].get()->getPointData()->worldMat = *this->pointLights[i].get()->getTransformWorld();
		// Double check so that it actually gets correct values.
		this->activePointLights.pointLight[i] = *this->pointLights[i].get()->getPointData();
	}
	memcpy(dataPtr.pData, &this->activePointLights, sizeof(ACTIVEPOINTLIGHTS));
	this->gDeviceContext->Unmap(this->pointLightDataBuffer.Get(), 0);
}
void DX::RenderLightPass()
{
	this->gDeviceContext->OMSetRenderTargets(1, this->gBackbufferRTV[this->currentFrame].GetAddressOf(), this->gDepthStencilView[this->currentFrame].Get());

	this->gDeviceContext->VSSetShader(this->gGBufferVertexShader.Get(), nullptr, 0);
	this->gDeviceContext->GSSetShader(nullptr, nullptr, 0);
	this->gDeviceContext->PSSetShader(this->gLightPassShader.Get(), nullptr, 0);
	
	this->UpdatePointLightBuffers();
	this->gDeviceContext->PSSetConstantBuffers(0, 1, this->cameraBuffer[0].GetAddressOf());
	this->gDeviceContext->PSSetConstantBuffers(1, 1, this->pointLightDataBuffer.GetAddressOf());

	this->gDeviceContext->PSSetShaderResources(0, 1, this->gGBufferSRVs[0].GetAddressOf());
	this->gDeviceContext->PSSetShaderResources(1, 1, this->gGBufferSRVs[1].GetAddressOf());
	this->gDeviceContext->PSSetShaderResources(2, 1, this->gGBufferSRVs[2].GetAddressOf());

	this->gDeviceContext->PSSetSamplers(0, 1, this->txSamplerState.GetAddressOf());

	for (UINT i = 0; i < this->meshes.size(); ++i)
	{
		for (UINT j = 0; j < meshes[i].get()->getFaceCount(); ++j)
		{
			UINT32 vertexCount{ this->meshes[i].get()->getVertexCount(j) };
			ID3D11Buffer* vertexBuffer{ this->meshes[i].get()->getVertexBuffer(j) };
			ID3D11Buffer* vertexIDBuffer{ this->meshes[i].get()->getVertexIDBuffer(j) };

			UINT vertexStride{ sizeof(NODETYPES::Mesh::VERTEX) };
			UINT vertexOffset{};
			this->gDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);
			this->gDeviceContext->IASetIndexBuffer(vertexIDBuffer, DXGI_FORMAT_R32_UINT, 0);

			this->gDeviceContext->DrawIndexed(vertexCount, 0, 0);
		}
	}

	this->gDeviceContext->PSSetConstantBuffers(0, 0, nullptr);
	this->gDeviceContext->PSSetConstantBuffers(1, 0, nullptr);

	this->gDeviceContext->PSSetShaderResources(0, 1, &this->nullSRV);
	this->gDeviceContext->PSSetShaderResources(1, 1, &this->nullSRV);
	this->gDeviceContext->PSSetShaderResources(2, 1, &this->nullSRV);

	this->gDeviceContext->PSSetSamplers(0, 0, nullptr);
}
void DX::clearRender()
{
	float clearColor[] = { 0.f, 0.f, 0.0f, 1.0f };

	this->gDeviceContext->ClearRenderTargetView(this->gBackbufferRTV[this->currentFrame].Get(), clearColor);
	this->gDeviceContext->ClearDepthStencilView(this->gDepthStencilView[this->currentFrame].Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
void DX::updateGBufferShaders(size_t i)
{
	if (this->meshes[i].get()->existShader())
	{
		if (this->meshes[i].get()->existDiffuse())
		{
			ID3D11ShaderResourceView* DiffuseSRV{ this->meshes[i]->getDiffuseBuffer() };
			if (DiffuseSRV)
			{
				this->gDeviceContext->PSSetShaderResources(0, 1, &DiffuseSRV);
			}
			else
			{
				this->gDeviceContext->PSSetShaderResources(0, 1, this->defaultTextureSRV.GetAddressOf());
			}
		}
		else
		{
			this->gDeviceContext->PSSetShaderResources(0, 1, this->defaultTextureSRV.GetAddressOf());
		}

		if (this->meshes[i].get()->existNormal())
		{
			ID3D11ShaderResourceView* NormalSRV{ this->meshes[i]->getNormalBuffer() };
			if (NormalSRV)
			{
				this->gDeviceContext->PSSetShaderResources(1, 1, &NormalSRV);
			}
			else
			{
				this->gDeviceContext->PSSetShaderResources(1, 1, this->defaultTextureSRV.GetAddressOf());
			}
		}
		else
		{
			this->gDeviceContext->PSSetShaderResources(1, 1, this->defaultTextureSRV.GetAddressOf());
		}
	}
	else
	{
		this->gDeviceContext->PSSetShaderResources(0, 1, this->defaultTextureSRV.GetAddressOf());
		this->gDeviceContext->PSSetShaderResources(1, 1, this->defaultTextureSRV.GetAddressOf());
	}
}
void DX::UpdateTransformBuffer(size_t index)
{
	D3D11_MAPPED_SUBRESOURCE dataPtr {};

	this->gDeviceContext->Map(this->transformBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	memcpy(dataPtr.pData, this->meshes[index].get()->getTransformWorld(), sizeof(DirectX::XMMATRIX));
	this->gDeviceContext->Unmap(this->transformBuffer.Get(), 0);
}
void DX::UpdateCameraBuffers()
{
	D3D11_MAPPED_SUBRESOURCE dataPtr {};

	this->gDeviceContext->Map(this->cameraBuffer[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	memcpy(dataPtr.pData, this->activeCamera->getViewMatix(), sizeof(DirectX::XMMATRIX));
	this->gDeviceContext->Unmap(this->cameraBuffer[0].Get(), 0);
	ZeroMemory(&dataPtr, sizeof(dataPtr));

	this->gDeviceContext->Map(this->cameraBuffer[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dataPtr);
	memcpy(dataPtr.pData, this->activeCamera->getProjectionMatrix(), sizeof(DirectX::XMMATRIX));
	this->gDeviceContext->Unmap(this->cameraBuffer[1].Get(), 0);
}
void DX::RenderGBuffer()
{
	this->gDeviceContext->OMSetRenderTargets(3, this->gGBufferRTVs->GetAddressOf(), this->gDepthStencilView[this->currentFrame].Get());
	//this->gDeviceContext->OMSetDepthStencilState(this->gDepthStencilState.Get(), 1);

	this->gDeviceContext->VSSetShader(this->gGBufferVertexShader.Get(), nullptr, 0);
	//this->gDeviceContext->GSSetShader(this->gGBufferGeometryShader.Get(), nullptr, 0);
	this->gDeviceContext->PSSetShader(this->gGBufferPixelShader.Get(), nullptr, 0);
	
	this->gDeviceContext->PSSetSamplers(0, 1, this->txSamplerState.GetAddressOf());

	this->UpdateCameraBuffers();

	for (UINT i = 0; i < this->meshes.size(); ++i)
	{
		this->UpdateTransformBuffer(i);
		this->updateGBufferShaders(i);
		
		this->gDeviceContext->VSSetConstantBuffers(0, 1, this->cameraBuffer[0].GetAddressOf());
		this->gDeviceContext->VSSetConstantBuffers(1, 1, this->cameraBuffer[1].GetAddressOf());
		this->gDeviceContext->VSSetConstantBuffers(2, 1, this->transformBuffer.GetAddressOf());

		for (UINT j = 0; j < meshes[i].get()->getFaceCount(); ++j)
		{
			this->updateVertexBuffers(i, j);
			
			UINT32 vertexCount{ this->meshes[i].get()->getVertexCount(j) };
			ID3D11Buffer* vertexBuffer{ this->meshes[i].get()->getVertexBuffer(j) };
			ID3D11Buffer* vertexIDBuffer{ this->meshes[i].get()->getVertexIDBuffer(j) };

			UINT vertexStride{ sizeof(NODETYPES::Mesh::VERTEX) };
			UINT vertexOffset{};
			this->gDeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);
			this->gDeviceContext->IASetIndexBuffer(vertexIDBuffer, DXGI_FORMAT_R32_UINT, 0);

			this->gDeviceContext->DrawIndexed(vertexCount, 0, 0);
		}
	}

	ID3D11ShaderResourceView* nullSRV {};
	this->gDeviceContext->PSSetShaderResources(0, 1, &this->nullSRV);
	this->gDeviceContext->PSSetShaderResources(1, 1, &this->nullSRV);
}
void DX::ClearGBufferTargets()
{
	float clearColor[] { 0.17f, 0.84f, 0.9f, 1.0f };
	float clearColor2[]{ 0.25f, 0.0f, 0.84f, 1.0f };
	float clearColor3[]{ 0.0f, 0.5f, 1.0f, 1.0f };

	this->gDeviceContext->ClearRenderTargetView(this->gGBufferRTVs[0].Get(), clearColor);
	this->gDeviceContext->ClearRenderTargetView(this->gGBufferRTVs[1].Get(), clearColor2);
	this->gDeviceContext->ClearRenderTargetView(this->gGBufferRTVs[2].Get(), clearColor3);
}
void DX::Render()
{
	this->gDeviceContext->IASetInputLayout(this->gVertexLayout.Get());
	this->gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	this->gDeviceContext->RSSetState(this->gRasterizerState.Get());
	this->ClearGBufferTargets();
	this->RenderGBuffer();

	//this->clearRender();
	//this->RenderLightPass();

	this->gSwapchain->Present(0, 0);
}
