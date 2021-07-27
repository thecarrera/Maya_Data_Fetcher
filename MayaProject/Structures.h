#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>

struct ObjectRenameData
{
	std::string lastName;
	std::string newName;
};

struct CameraData
{
	std::string uuid = "None";
	std::string camName = "None";
	std::vector<std::string> camTransformerNames;
	std::vector<std::string> camTransformerUuids;
	double projMat[4][4] = { 0 };
	double ARO;
	double HFOV;
	double VFOV;
	double nPlane;
	double fPlane;
};

struct TransformerData
{
	std::string transformerUuid = "None";
	std::string transformerName = "None";
	double transformMatrix[4][4] = { 0 };
	std::vector<std::string> parentNames;
};

struct MeshData
{
	struct VertexID
	{
		int vertexID = 0;
		int normalID = 0;
	};

	struct Vertex
	{
		double VertexX = 0;
		double VertexY = 0;
		double VertexZ = 0;
	};

	struct Normals
	{
		float NormalX = 0;
		float NormalY = 0;
		float NormalZ = 0;
	};

	struct UV
	{
		float U = 0;
		float V = 0;
	};

	struct UVset
	{
		std::string uvSetName = "none";
		std::vector<UV> uvPoints;
		std::vector<int> uvDrawOrder;
	};

	std::string meshUuid = "none";
	std::string meshName = "none";
	std::string meshTransform = "none";

	//If mesh exist, compare count with old to see if 
	//buffer needs re-alloc.
	UINT vertexCount = 0;
	std::vector<Vertex> vertexList;
	std::vector<Normals> normalsList;

	//Supports multi-texturing
	std::vector<UVset> uvSets;

	std::vector<VertexID> VertexDrawOrder;
};

struct MaterialData
{
	std::string texturePath = "None";
	std::vector<float> color;
	std::vector<float> ambientColor;
	std::vector<float> transparency;

};