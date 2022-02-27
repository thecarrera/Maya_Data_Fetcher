#include "Nodes.hpp"
//##### NODE
void NODETYPES::Node::removeReference(std::string uuid) {
	for (size_t i = 0; i < this->referredBy.size(); ++i) {
		if (this->referredBy[i].first->uuid == uuid) {
			this->referredBy.erase(this->referredBy.begin() + i);
		}
	}
}
void NODETYPES::Node::removeReferences()
{
	for (size_t i = 0; i < this->referredBy.size(); ++i) {
		std::string type{ this->referredBy[i].first->getType() };
		if (type == "mesh") {
			NODETYPES::Mesh* mesh{ dynamic_cast<NODETYPES::Mesh*>(this->referredBy[i].first) };
			if (this->type == "transform") {
				mesh->removeTransformReference();
			}
			else if (this->type == "shadingEngine") {
				mesh->removeShadingEngine(this->uuid);
			}
		}
		else if (type == "camera") {
			NODETYPES::Camera* camera{ dynamic_cast<NODETYPES::Camera*>(this->referredBy[i].first) };
			if (this->type == "transform") {
				camera->removeTransformMatrixReference();
			}
		}
		else if (type == "pointLight") {
			NODETYPES::PointLight* pointLight{ dynamic_cast<NODETYPES::PointLight*>(this->referredBy[i].first) };
			if (this->type == "transform") {
				pointLight->removeTransformReference();
			}
		}
		else if (type == "bump2d") {
			NODETYPES::Bump* bump{ dynamic_cast<NODETYPES::Bump*>(this->referredBy[i].first) };
			if (this->type == "file") {
				bump->removeTexture(this->uuid);
			}
		}
		else if (type == "lambert") {
			NODETYPES::Lambert* lambert{ dynamic_cast<NODETYPES::Lambert*>(this->referredBy[i].first) };
			if (this->type == "file") {
				if (this->referredBy[i].second == "DiffuseMap") {
					lambert->removeDiffuseMap(this->uuid);
				}
				else if (this->referredBy[i].second == "ColorMap") {
					lambert->removeColorMap(this->uuid);
				}
				else if (this->referredBy[i].second == "TransparencyMap") {
					lambert->removeTransparancyMap(this->uuid);
				}
			}
			else if (this->type == "bump2d") {
				lambert->removeNormalMap(this->uuid);
			}
		}
		else if (type == "blinn") {
			NODETYPES::Blinn* blinn{ dynamic_cast<NODETYPES::Blinn*>(this->referredBy[i].first) };
			if (this->type == "file") {
				if (this->referredBy[i].second == "DiffuseMap") {
					blinn->removeDiffuseMap(this->uuid);
				}
				else if (this->referredBy[i].second == "ColorMap") {
					blinn->removeColorMap(this->uuid);
				}
				else if (this->referredBy[i].second == "TransparencyMap") {
					blinn->removeTransparancyMap(this->uuid);
				}
			}
			else if (this->type == "bump2d") {
				blinn->removeNormalMap(this->uuid);
			}
		}
		else if (type == "shadingEngine") {
			NODETYPES::ShadingEngine* shadingEngine{ dynamic_cast<NODETYPES::ShadingEngine*>(this->referredBy[i].first) };
			//if (this->type == "mesh")
			//{
			//	shadingEngine->removeMesh(this->uuid);
			//}
			if (this->type == "lambert" || this->type == "blinn") {
				shadingEngine->removeMaterial(this->uuid);
			}
		}
	}
	this->referredBy.clear();
}

//#### Mesh
bool NODETYPES::Mesh::existShader() {
	if (this->shadingEngines.size() > 0)
	{
		return dynamic_cast<NODETYPES::ShadingEngine*>(this->shadingEngines[0])->getMaterialExist();
	}
	else
	{
		return false;
	}
}
bool NODETYPES::Mesh::existDiffuse() { 
	return dynamic_cast<NODETYPES::ShadingEngine*>(this->shadingEngines[0])->getDiffuseExist(); 
}
bool NODETYPES::Mesh::existNormal() {
	return dynamic_cast<NODETYPES::ShadingEngine*>(this->shadingEngines[0])->getNormalExist();
}
ID3D11ShaderResourceView* NODETYPES::Mesh::getDiffuseBuffer() {
	return dynamic_cast<NODETYPES::ShadingEngine*>(this->shadingEngines[0])->getDiffuseTexture();
}
ID3D11ShaderResourceView* NODETYPES::Mesh::getDefaultDiffuseBuffer()
{
	return dynamic_cast<NODETYPES::ShadingEngine*>(this->shadingEngines[0])->getDefaultDiffuseTexture();
}
ID3D11ShaderResourceView* NODETYPES::Mesh::getNormalBuffer() {
	return dynamic_cast<NODETYPES::ShadingEngine*>(this->shadingEngines[0])->getNormalTexture();
}
ID3D11ShaderResourceView* NODETYPES::Mesh::getDefaultNormalBuffer()
{
	return dynamic_cast<NODETYPES::ShadingEngine*>(this->shadingEngines[0])->getDefaultNormalTexture();
}

//##### Transform
void NODETYPES::Transform::clearParents() {
	for (size_t i = 0; i < this->parents.size(); ++i)
	{
		NODETYPES::Transform* transform{ dynamic_cast<NODETYPES::Transform*>(parents[i]) };
		for (size_t j = 0; j < transform->children.size(); ++j)
		{
			if (transform->children[j]->getUuid() == this->getUuid())
			{
				transform->children.erase(transform->children.begin() + j);
			}
		}
	}
	this->parents.clear();
}
void NODETYPES::Transform::clearChildren() {
	for (size_t i = 0; i < this->children.size(); ++i)
	{
		NODETYPES::Transform* transform{ dynamic_cast<NODETYPES::Transform*>(children[i]) };
		for (size_t j = 0; j < transform->parents.size(); ++j)
		{
			if (transform->parents[j]->getUuid() == this->getUuid())
			{
				transform->parents.erase(transform->parents.begin() + j);
			}
		}
	}
	this->children.clear();
}


//##### Mesh
void NODETYPES::Mesh::clearShadingEngines()
{
	for (size_t i = 0; i < this->shadingEngines.size(); ++i)
	{
		this->shadingEngines[i]->removeReference(this->getUuid());
	}
}
void NODETYPES::Mesh::clearTransformReference() {
	this->transformer->removeReference(this->getUuid());
}



//##### 