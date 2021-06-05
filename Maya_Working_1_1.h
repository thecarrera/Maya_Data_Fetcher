// ###########################################################################################################
// #																										 #
// #	Dependency Graph(Root Node Table) -> DagPath (path to node)											 #
// #			|																							 #
// #			v							  (same, but different)											 #
// #	     MObject (pointer to all data) -> DAG Node (exists before node)									 #
// #			Type: MFnMesh				   		   (Contains parenting and pre data 					 #
// #				  MFnTransform			  																 #
// #				  etc...			   -> Node (Shape/Transform Node)	->	Attributes/Plugs			 #
// #											   (Empty upon creation)									 #
// #																										 #
// # ------------------------------------------------------------------------------------------------------- #
// #					MFnDagPath																			 #
// #	Dependency Graph  ---|			(M3dView = maya viewport)											 #
// #	*********************|***************************************************************************	 #
// #	*					 v-------------------------------------------------------->					*	 #
// #	*															MObject		(ex)Type: MFnMesh		*	 #
// #	*															~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	*	 #
// #	*	DAG Node: Cube1			 								~  DAG Node:				   ~	*	 #
// #	*	**************************								~  **************************  ~	*	 #
// #	*	*			^			*								~  *						*  ~	*	 #
// #	*	************|*************								~  **************************  ~	*	 #
// #	*				|			 								~ 							   ~	*	 #
// #	*	DG Node:	|				 							~  DG Node:					   ~	*	 #
// #	*	************|********************						   **************************  ~	*	 #
// #	*	*			v	Attributes:		* Plugs:				   *						*  ~	*	 #
// #	*  O*					outColor:	*    O--------------------O* Color					O  ~	*	 #
// #	*	*								*						   *						*  ~	*	 #
// #	*  O*					Pos:		*    O----------|		  O* image					O  ~	*	 #
// #	*	*								*	 		    |		   *						*  ~	*	 #
// #	*  O*								*    			|---------O* Pos					O  ~	*	 #
// #	*	*								*						   *						*  ~	*	 #
// #	*	*		Compute()				*						   *						*  ~	*	 #
// #	*	*********************************						~  **************************  ~	*	 #
// #	*															~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	*	 #
// #	*************************************************************************************************	 #
// #																										 #
// # ------------------------------------------------------------------------------------------------------- #
// #																										 #
// #	Data updates per pulse (render loop) read "Complete Maya Programing v.1" p.40-47 ch2.2.10			 #
// #	Dirty = old data/no data (not updated)																 #
// #																										 #
// ###########################################################################################################

// Hindsight notes:
//	* Plugs data is obtainable through DataHandles/DataBlocks
//	* Is callbacks to transformers necessary? Data seem to always be available. Callbacks added to revieve smaller changes
//	
//  * Separate further! Parenting | micro data. Send smaller amount of data!
//

// DOES NOT SUPPORT REDO/UNDO!!!
// DOES NOT SUPPORT REDO/UNDO!!!
// DOES NOT SUPPORT REDO/UNDO!!!
// DOES NOT SUPPORT REDO/UNDO!!!

#include "maya_includes.h"
#include "ComLib.h"

MCallbackIdArray myCallbackArray;
ComLib comlib("sharedFileMap", 100000 * 1 << 20);

void triangulateList(MIntArray Count, MIntArray& List)
{
	MIntArray newList;
	int vertexCount;
	int ListPos = 0;
	int faceLeftOvers = 0;

	for (UINT i = 0; i < Count.length(); ++i)
	{
		faceLeftOvers = Count[i] % 4;
		vertexCount = Count[i];

		for (UINT j = 0; j < int(Count[i]/4); ++j)
		{
			newList.append(List[ListPos]);
			newList.append(List[ListPos + 1]);
			newList.append(List[ListPos + 2]);

			newList.append(List[ListPos + 2]);
			newList.append(List[ListPos + 1]);
			newList.append(List[ListPos + 3]);

			ListPos += 4;
		}
		int listSize = List.length();
		switch (faceLeftOvers)
		{
		case 1:
			//ListPos += 1;
			if (List[ListPos - 2] < List[ListPos - 1])
			{
				newList.append(List[ListPos - 2]);
				newList.append(List[ListPos - 1]);
				newList.append(List[ListPos]);

			}
			else
			{
				newList.append(List[ListPos - 1]);
				newList.append(List[ListPos - 2]);
				newList.append(List[ListPos]);
			}
			break;
		case 2:
			ListPos += 1;
			if (List[ListPos - 2] < List[ListPos - 1])
			{
				newList.append(List[ListPos - 2]);
				newList.append(List[ListPos - 1]);
				newList.append(List[ListPos]);

			}
			else
			{
				newList.append(List[ListPos - 1]);
				newList.append(List[ListPos - 2]);
				newList.append(List[ListPos]);
			}
			break;
		default:
			break;
		}

	}

	List.clear();
	List = newList;
}

//Split
//void outputMeshData(MFnMesh& mesh)
//{
//	if (mesh.name().length() != 0)
//	{
//		ComLib::MeshData meshData;
//		MStatus status;
//
//		//Mesh name and ID is reserved when creation of the node.
//		MFnTransform parentTransform(mesh.parent(0));
//		meshData.meshUuid = mesh.uuid().asString().asChar();
//		meshData.meshName = mesh.name().asChar();
//		MGlobal::displayInfo(mesh.name().asChar());
//		meshData.meshTransform = parentTransform.name().asChar();
//
//		MString debugString;
//		debugString = "##########################";
//		MGlobal::displayInfo(debugString);
//
//		// Vertex pos
//		MFloatPointArray vertexList;
//		mesh.getPoints(vertexList, MSpace::kObject);
//
//		// Vertex count per polygon ("face") AFFECTED BY TRIANGULATE! | vertexList ("vertex ID") Triangle-based
//		MIntArray triangleCount;
//		MIntArray triangleList;
//		mesh.getTriangles(triangleCount, triangleList);
//
//		//for (UINT i = 0; i < triangleList.length(); ++i)
//		//{
//		//	debugString = triangleList[i];
//		//	MGlobal::displayInfo(debugString);
//		//}
//
//		//Gets the holes verticies
//		MIntArray holesInfo;
//		MIntArray holesList;
//		mesh.getHoles(holesInfo, holesList);
//		
//		//Normals
//		MFloatVectorArray normals;
//		MIntArray normalCount;
//		MIntArray normalList;
//		mesh.getNormals(normals, MSpace::kObject);
//		mesh.getNormalIds(normalCount, normalList);
//
//		//UVs sets | per polygon / face uv count AFFECTED BY TRIANGULATE! | UVIds
//		MStringArray uvSets;
//		MIntArray UVCounts;
//		MIntArray UVList;
//		mesh.getUVSetNames(uvSets);
//
//		if (vertexList.length() <= 0)
//		{
//			return;
//		}
//		else
//		{
//			debugString = "Verticies: ";
//			debugString += vertexList.length();
//			MGlobal::displayInfo(debugString);
//
//			ComLib::MeshData::Vertex vertex;
//
//			for (UINT i = 0; i < vertexList.length(); ++i)
//			{
//				vertex.VertexX = vertexList[i].x;
//				vertex.VertexY = vertexList[i].y;
//				vertex.VertexZ = vertexList[i].z;
//
//				debugString = "Vertex: ";
//				debugString += vertexList[i].x;
//				debugString += vertexList[i].y;
//				debugString += vertexList[i].z;
//				debugString += "\n";
//
//				meshData.vertexList.push_back(vertex);
//			}
//		}
//
//		if (triangleCount.length() != (triangleList.length() / 3))
//		{
//			triangulateList(normalCount, normalList);
//		}
//
//		ComLib::MeshData::Normals comNormal;
//		for (size_t i = 0; i < normals.length(); ++i)
//		{			
//			comNormal.NormalX = normals[i].x;
//			comNormal.NormalY = normals[i].y;
//			comNormal.NormalZ = normals[i].z;
//			
//			meshData.normalsList.push_back(comNormal);
//		}
//
//		ComLib::MeshData::VertexID tempIDs;
//		for (UINT i = 0; i < triangleList.length(); ++i)
//		{
//			tempIDs.vertexID = triangleList[i];
//			tempIDs.normalID = normalList[i];
//			meshData.VertexDrawOrder.push_back(tempIDs);
//		}
//
//		if (uvSets.length() <= 0 || mesh.numUVs(uvSets[0]) <= 0)
//		{
//			//add existing UVs false
//		}
//		else
//		{
//			float2 uvPoint;
//			MPoint vertexPoint;
//			ComLib::MeshData::UV uv;
//			std::vector<ComLib::MeshData::UV> UVTempList;
//			for (UINT i	= 0; i < uvSets.length(); ++i)
//			{
//				UVTempList.clear();
//				for (UINT j = 0; j < vertexList.length(); ++j)
//				{
//					vertexPoint = vertexList[j];
//					mesh.getUVAtPoint(vertexPoint, uvPoint, MSpace::kObject, &uvSets[i]);
//					uv.U = uvPoint[0];
//					uv.V = uvPoint[1];
//					UVTempList.push_back(uv);
//				}
//				meshData.uvPoints.push_back(UVTempList);
//			}
//		}
//
//		//for (UINT i = 0; i < vertexList.length(); ++i)
//		//{
//		//	
//		//}
//
//		//debugString = "Triangle faces: ";
//		//debugString += triangleCount.length();
//		//MGlobal::displayInfo(debugString);
//
//		//debugString = "Triangle IDs: ";
//		//debugString += triangleList.length();
//		//MGlobal::displayInfo(debugString);
//
//		//debugString = "normals: ";
//		//debugString += UINT(meshData.normalsList.size());
//		//MGlobal::displayInfo(debugString);
//
//		//debugString = "Normal per face: ";
//		//debugString += normalCount.length();
//		//MGlobal::displayInfo(debugString);
//
//		//debugString = "Normal IDs: ";
//		//debugString += normalList.length();
//		//MGlobal::displayInfo(debugString);
//
//		//debugString = "UVs: ";
//		//debugString += UINT(meshData.uvList.size());
//		//MGlobal::displayInfo(debugString);
//
//		//debugString = "##########################";
//		//MGlobal::displayInfo(debugString);
//		
//		MObjectArray shaderArray;
//		MIntArray faceIndices;
//
//		// Gets shaders of first instance
//		mesh.getConnectedShaders(0, shaderArray, faceIndices);
//		for (UINT i = 0; i < shaderArray.length(); ++i)
//		{
//			//initialShadingGroup
//			MFnDependencyNode shaderGroup(shaderArray[i]);
//
//			MPlug shaderPlug = shaderGroup.findPlug("surfaceShader", status);
//			if (status == MS::kSuccess)
//			{
//				MPlugArray shaderConnections;
//				shaderPlug.connectedTo(shaderConnections, true, false);
//				for (UINT j = 0; j < shaderConnections.length(); ++j)
//				{
//					//Shader Material .outColor plug
//					MPlug outColorPlug = shaderConnections[j];
//					//Shader Material (Lambert/Blinn)
//					MFnDependencyNode materialNode(outColorPlug.node());
//
//					//*material*.color
//					MPlug colorPlug = materialNode.findPlug("color", status);
//
//					MItDependencyGraph dgIt(
//						colorPlug,
//						MFn::kFileTexture,
//						MItDependencyGraph::kUpstream,
//						MItDependencyGraph::kBreadthFirst,
//						MItDependencyGraph::kNodeLevel,
//						&status
//					);
//					dgIt.disablePruningOnFilter();
//
//					if (!dgIt.isDone())
//					{
//						MObject textureObject = dgIt.currentItem();
//						//Image Name: (plug)				How do I find the name fileTextureName?
//						MPlug fileNamePlug = MFnDependencyNode(textureObject).findPlug("fileTextureName", status);
//						if (status == MS::kSuccess)
//						{
//							MString texturePath;
//							fileNamePlug.getValue(texturePath);
//
//							//Add to meshData
//						}
//					}
//
//				}
//			}
//		}
//
//
//		//previous function used the bool as return value? Perhaps to not load texture if there was no UV coordinates?
//
//		//Might need to calculate vector<string> * item count if project crashes here
//		//comlib.send(&meshData, ComLib::MSG_TYPE::MESH, sizeof(ComLib::MeshData));
//	}
//}
//---------------------------------------------------------------------------------------------------------------------------------





//Needs change
void nameChangeCallback(MObject& node, const MString &lastName, void* clientData)
{
	// Fetches the name of the object that has been changed.
	if (node.hasFn(MFn::kMesh))
	{
		MFnMesh mesh(node);
		MString msg(mesh.name());


		/*std::map<std::string, int> Options;
		Options.insert(std::make_pair("xyz", 0));

		std::map<std::string, int>::iterator iter;
		iter = Options.find(user_opt);

		if (iter != Options.end())
			iter->second++;*/

			//Fetches names of objects that uses this transformer
		int childCount = mesh.childCount();

		for (int i = 0; i < childCount; ++i)
		{
			if (mesh.child(i).apiType() == MFn::kMesh)
			{
				MFnMesh mesh = mesh.child(i);
				MString meshName = mesh.name();
				//MGlobal::displayInfo("Connected Mesh Name: " + meshName);
			}
			/*else if (transformer.child(i).apiType() == MFn::kCamera)
			{
			}
			else if (transformer.child(i).apiType() == MFn::kLight)
			{
			}*/
		}
	}
	else if (node.hasFn(MFn::kTransform))
	{
		MFnTransform transform(node);
		MString msg(transform.name());
		//MGlobal::displayInfo("-> Rename: " +  msg);
		//MGlobal::displayInfo("->LastName: " + lastName);
	}
}

void renderChangeCallback(const MString &str, void *clientData)
{
	//M3dView sceneView;
	//sceneView = sceneView.active3dView();
	//MDagPath camDagPath;
	//sceneView.getCamera(camDagPath);
	//MFnCamera cam(camDagPath.node());
	//MFnTransform camTransform = cam.parent(0);

	////getActiveCamera();
}

void getMaterial(MFnMesh& mesh)
{
	MStatus res;
	MString debugString;
	MObjectArray shaderArray;
	MIntArray faceIndices;

	mesh.getConnectedShaders(0, shaderArray, faceIndices);
	for (UINT i = 0; i < shaderArray.length(); ++i)
	{
		//initialShadingGroup
		MFnDependencyNode shaderGroup(shaderArray[i]);

		MPlug shaderPlug = shaderGroup.findPlug("surfaceShader", res);
		if (res
			== MS::kSuccess)
		{
			MPlugArray shaderConnections;
			shaderPlug.connectedTo(shaderConnections, true, false);
			for (UINT j = 0; j < shaderConnections.length(); ++j)
			{
				//Shader Material .outColor plug
				MPlug outColorPlug = shaderConnections[j];
				//Shader Material (Lambert/Blinn)
				MFnDependencyNode materialNode(outColorPlug.node());
				MGlobal::displayInfo("-1-");
				MGlobal::displayInfo(materialNode.name());
				MGlobal::displayInfo(materialNode.typeName());

				for (size_t k = 0; k < materialNode.attributeCount(); ++k)
				{
					MFnAttribute tempf = materialNode.attribute(k);
					MGlobal::displayInfo(tempf.name());
				}

				// Fetch the plug data: 
				//*material*.color
				MPlug colorPlug = materialNode.findPlug("color", res);
				if (colorPlug.node().apiType() == MFn::kData3Float)
				{
					MDataHandle dataHandle = {};
					colorPlug.getValue(dataHandle);
					MFloatVector color = dataHandle.asFloatVector();
					debugString = "R: ";
					debugString += color.x;
					debugString += "G: ";
					debugString += color.y;
					debugString += "B: ";
					debugString += color.z;
					MGlobal::displayInfo(debugString);
					//
					//<----- Add to meshData
					//
				}

				//*material*.ambientColor
				MPlug ambientColorPlug = materialNode.findPlug("ambientColor", res);
				MGlobal::displayInfo("-2-");
				if (ambientColorPlug.node().apiType() == MFn::kData3Float)
				{
					MDataHandle dataHandle = {};
					ambientColorPlug.getValue(dataHandle);
					MFloatVector color = dataHandle.asFloatVector();
					debugString = "R: ";
					debugString += color.x;
					debugString += "G: ";
					debugString += color.y;
					debugString += "B: ";
					debugString += color.z;
					MGlobal::displayInfo(debugString);
					//
					//<----- Add to meshData
					//
				}

				//*material*.transparency
				MPlug transparencyPlug = materialNode.findPlug("transparency", res);
				MGlobal::displayInfo("-3-");
				if (transparencyPlug.node().apiType() == MFn::kData3Float)
				{
					MDataHandle dataHandle = {};
					transparencyPlug.getValue(dataHandle);
					MFloatVector color = dataHandle.asFloatVector();
					debugString = "R: ";
					debugString += color.x;
					debugString += "G: ";
					debugString += color.y;
					debugString += "B: ";
					debugString += color.z;
					MGlobal::displayInfo(debugString);
					//
					//<----- Add to meshData
					//
				}

				//Get connected textures
				MItDependencyGraph dgIt(
					colorPlug,
					MFn::kFileTexture,
					MItDependencyGraph::kUpstream,
					MItDependencyGraph::kBreadthFirst,
					MItDependencyGraph::kNodeLevel,
					&res
				);
				dgIt.disablePruningOnFilter();

				if (!dgIt.isDone())
				{
					MObject textureObject = dgIt.currentItem();
					//Image Name: (plug)
					MPlug fileNamePlug = MFnDependencyNode(textureObject).findPlug("fileTextureName", res);
					if (res == MS::kSuccess)
					{
						MString texturePath;
						fileNamePlug.getValue(texturePath);
						MGlobal::displayInfo(texturePath);

						//
						//<----- Add to meshData
						//
					}
				}

			}
		}
	}
}

void getUVs(MFnMesh& mesh)
{
	ComLib::MeshData meshData;
	MStatus status;
	MString debugString;

	meshData.meshUuid = mesh.uuid().asString().asChar();
	meshData.meshName = mesh.name().asChar();

	MFloatArray Us;
	MFloatArray Vs;
	ComLib::MeshData::UV uv;
	ComLib::MeshData::UVset UVset;
	MString uvSetName;

	MStringArray uvSets;
	mesh.getUVSetNames(uvSets);

	if (!(uvSets.length() <= 0 || mesh.numUVs(uvSets[0]) <= 0))
	{

		for (UINT i = 0; i < uvSets.length(); ++i)
		{
			UVset.uvSetName = uvSets[i].asChar();
			//MGlobal::displayInfo(uvSets[i].asChar());

			mesh.getUVs(Us, Vs, &uvSets[i]);
			debugString = "Uv Length: ";
			debugString += Us.length();
			//MGlobal::displayInfo(debugString);
			for (UINT j = 0; j < Us.length(); j++)
			{
				uv.U = Us[j];
				uv.V = Vs[j];
				UVset.uvPoints.push_back(uv);
				debugString = "U: ";
				debugString += uv.U;
				debugString += " V: ";
				debugString += uv.V;
				//MGlobal::displayInfo(debugString);
			}
		}
	}
}

void getNormals(MFnMesh& mesh)
{
	ComLib::MeshData meshData;
	MStatus status;
	MString debugString;

	meshData.meshUuid = mesh.uuid().asString().asChar();
	meshData.meshName = mesh.name().asChar();

	MIntArray triangleCount;
	MIntArray triangleList;
	mesh.getTriangles(triangleCount, triangleList);

	//Normals
	MFloatVectorArray normals;
	MIntArray normalCount;
	MIntArray normalList;
	mesh.getNormals(normals, MSpace::kObject);
	mesh.getNormalIds(normalCount, normalList);

	if (triangleCount.length() != (triangleList.length() / 3))
	{
		triangulateList(normalCount, normalList);
	}

	ComLib::MeshData::Normals comNormal;
	for (size_t i = 0; i < normals.length(); ++i)
	{
		comNormal.NormalX = normals[i].x;
		comNormal.NormalY = normals[i].y;
		comNormal.NormalZ = normals[i].z;

		meshData.normalsList.push_back(comNormal);
	}
}

//Finnish off
void sendPoints(MFnMesh& mesh)
{
	ComLib::MeshData meshData;
	MStatus status;
	MString debugString;

	meshData.meshUuid = mesh.uuid().asString().asChar();
	meshData.meshName = mesh.name().asChar();

	// Vertex pos
	MFloatPointArray vertexList;
	mesh.getPoints(vertexList, MSpace::kObject);

	if (vertexList.length() <= 0)
	{
		return;
	}
	else
	{
		ComLib::MeshData::Vertex vertex;

		for (UINT i = 0; i < vertexList.length(); ++i)
		{
			vertex.VertexX = vertexList[i].x;
			vertex.VertexY = vertexList[i].y;
			vertex.VertexZ = vertexList[i].z;

			meshData.vertexList.push_back(vertex);
		}
	}

}

//finish off
void getTotalVertexData(MFnMesh& mesh)
{
	ComLib::MeshData meshData;
	MStatus status;
	MString debugString;

	//Mesh name and ID is reserved when creation of the node.
	meshData.meshUuid = mesh.uuid().asString().asChar();
	meshData.meshName = mesh.name().asChar();

	// Vertex pos
	MFloatPointArray vertexList;
	mesh.getPoints(vertexList, MSpace::kObject);

	// Vertex count per polygon ("face") AFFECTED BY TRIANGULATE! | vertexList ("vertex ID") Triangle-based
	MIntArray triangleCount;
	MIntArray triangleList;
	mesh.getTriangles(triangleCount, triangleList);

	//Gets the holes verticies
	MIntArray holesInfo;
	MIntArray holesList;
	mesh.getHoles(holesInfo, holesList);

	if (vertexList.length() <= 0)
	{
		return;
	}
	else
	{
		ComLib::MeshData::Vertex vertex;

		for (UINT i = 0; i < vertexList.length(); ++i)
		{
			vertex.VertexX = vertexList[i].x;
			vertex.VertexY = vertexList[i].y;
			vertex.VertexZ = vertexList[i].z;

			meshData.vertexList.push_back(vertex);
		}
	}

	//Normals
	MFloatVectorArray normals;
	MIntArray normalCount;
	MIntArray normalList;
	mesh.getNormals(normals, MSpace::kObject);
	mesh.getNormalIds(normalCount, normalList);

	if (triangleCount.length() != (triangleList.length() / 3))
	{
		triangulateList(normalCount, normalList);
	}

	ComLib::MeshData::Normals comNormal;
	for (size_t i = 0; i < normals.length(); ++i)
	{
		comNormal.NormalX = normals[i].x;
		comNormal.NormalY = normals[i].y;
		comNormal.NormalZ = normals[i].z;

		meshData.normalsList.push_back(comNormal);
	}

	//UVs sets | per polygon / face uv count AFFECTED BY TRIANGULATE! | UVIds
	MFloatArray Us;
	MFloatArray Vs;
	ComLib::MeshData::UV uv;
	ComLib::MeshData::UVset UVset;
	MString uvSetName;

	MStringArray uvSets;
	mesh.getUVSetNames(uvSets);

	if (!(uvSets.length() <= 0 || mesh.numUVs(uvSets[0]) <= 0))
	{

		for (UINT i = 0; i < uvSets.length(); ++i)
		{
			UVset.uvSetName = uvSets[i].asChar();
			//MGlobal::displayInfo(uvSets[i].asChar());

			mesh.getUVs(Us, Vs, &uvSets[i]);
			debugString = "Uv Length: ";
			debugString += Us.length();
			//MGlobal::displayInfo(debugString);
			for (UINT j = 0; j < Us.length(); j++)
			{
				uv.U = Us[j];
				uv.V = Vs[j];
				UVset.uvPoints.push_back(uv);
				debugString = "U: ";
				debugString += uv.U;
				debugString += " V: ";
				debugString += uv.V;
				//MGlobal::displayInfo(debugString);
			}

			MItMeshPolygon faceIter(mesh.object());
			//MGlobal::displayInfo(debugString);
			UINT index = 0;
			for (; !faceIter.isDone(); faceIter.next())
			{
				int faceTriangles;
				faceIter.numTriangles(faceTriangles);

				debugString = "faceTriangles: ";
				debugString += faceTriangles;
				//MGlobal::displayInfo(debugString);

				debugString = "***************************";	
				//MGlobal::displayInfo(debugString);
				
				
				debugString = "polyVertexCount: ";
				debugString += faceIter.polygonVertexCount();
				
				for (UINT j = 0; j < faceTriangles * 3; ++j)
				{
					int uvID;
					debugString = "Face ID: ";
					debugString += faceIter.index();
					//MGlobal::displayInfo(debugString);
					debugString = "vert Index: ";
					debugString += index;
					//MGlobal::displayInfo(debugString);
					debugString = "Face vert index (j): ";
					debugString += triangleList[index];
					//MGlobal::displayInfo(debugString);

					mesh.getPolygonUVid(faceIter.index(), j, uvID, &uvSets[i]);
					UVset.uvDrawOrder.push_back(uvID);
					
					debugString = "UV ID: ";
					debugString += uvID;
					debugString += "\n###########";
					//MGlobal::displayInfo(debugString);

					index++;
				}
				debugString = "***************************";
			}

			debugString = "vertexList: ";
			debugString += triangleList.length();
			//MGlobal::displayInfo(debugString);
			debugString = "UVList size: ";
			debugString += static_cast<int>(UVset.uvDrawOrder.size());
			//MGlobal::displayInfo(debugString);
			for (UINT j = 0; j < UVset.uvDrawOrder.size(); j++)
			{
				debugString = UVset.uvDrawOrder[j];
				//MGlobal::displayInfo(debugString);
			}

			meshData.uvSets.push_back(UVset);
		}
	}

	ComLib::MeshData::VertexID tempIDs;
	for (UINT i = 0; i < triangleList.length(); ++i)
	{
		tempIDs.vertexID = triangleList[i];
		tempIDs.normalID = normalList[i];
		meshData.VertexDrawOrder.push_back(tempIDs);
	}
}

//Finish off
void getProjectionMatrix(MFnCamera& cam) 
{
	ComLib::CameraData camData;
	MObject camObject(cam.object());
	MStatus res;

	//Query cam name and projection matrix
	MString camName = cam.name();
	MString camUuid = cam.uuid();
	MFloatMatrix projectionMat = cam.projectionMatrix();

	//Query micro data (if needed, should be contained within the martix)
	double ARO = cam.aspectRatio();
	double HFOV = cam.horizontalFieldOfView();
	double VFOV = cam.verticalFieldOfView();
	double nPlane = cam.nearClippingPlane();
	double fPlane = cam.farClippingPlane();

	//Query view matrix name
	int parentCount = cam.parentCount();
	for (size_t i = 0; i < parentCount; ++i)
	{
		MFnTransform camTransform(cam.parent(i));
		MString camTransformerName = camTransform.name();
		MString camTransformerUuid = camTransform.uuid();
		camData.camTransformerUuids.push_back(camTransformerUuid.asChar());
		camData.camTransformerNames.push_back(camTransformerName.asChar());
	}

	// Fill the data for transfer to engine.
	camData.uuid = camUuid.asChar();
	camData.camName = camName.asChar();

	//ProjMat
	camData.projMat[0][0] = projectionMat.matrix[0][0];
	camData.projMat[1][0] = projectionMat.matrix[1][0];
	camData.projMat[2][0] = projectionMat.matrix[2][0];
	camData.projMat[3][0] = projectionMat.matrix[3][0];

	camData.projMat[0][1] = projectionMat.matrix[0][1];
	camData.projMat[1][1] = projectionMat.matrix[1][1];
	camData.projMat[2][1] = projectionMat.matrix[2][1];
	camData.projMat[3][1] = projectionMat.matrix[3][1];

	camData.projMat[0][2] = projectionMat.matrix[0][2];
	camData.projMat[1][2] = projectionMat.matrix[1][2];
	camData.projMat[2][2] = projectionMat.matrix[2][2];
	camData.projMat[3][2] = projectionMat.matrix[3][2];

	camData.projMat[0][3] = projectionMat.matrix[0][3];
	camData.projMat[1][3] = projectionMat.matrix[1][3];
	camData.projMat[2][3] = projectionMat.matrix[2][3];
	camData.projMat[3][3] = projectionMat.matrix[3][3];

	//Other Data
	camData.ARO = ARO;
	camData.HFOV = HFOV;
	camData.VFOV = VFOV;
	camData.nPlane = nPlane;
	camData.fPlane = fPlane;

	//comlib.send(&camData, ComLib::MSG_TYPE::ALLOCATECAMERA, sizeof(ComLib::CameraData));
}

//Finish off
void getTransfromData(MFnTransform& transformer)
{
	//Query the matrix and the node name
	MString transformerUuid = transformer.uuid();
	MFnDependencyNode transformerNode(transformer.object());
	MString transformerNodeName = transformerNode.name();
	MMatrix mat = transformer.transformation().asMatrix().matrix;
	
	// Fill the transform information
	ComLib::TransformerData transformData;
	transformData.transformerUuid = transformerUuid.asChar();
	transformData.transformerName = transformerNodeName.asChar();

	transformData.transformMatrix[0][0] = mat.matrix[0][0];
	transformData.transformMatrix[1][0] = mat.matrix[1][0];
	transformData.transformMatrix[2][0] = mat.matrix[2][0];
	transformData.transformMatrix[3][0] = mat.matrix[3][0];

	transformData.transformMatrix[0][1] = mat.matrix[0][1];
	transformData.transformMatrix[1][1] = mat.matrix[1][1];
	transformData.transformMatrix[2][1] = mat.matrix[2][1];
	transformData.transformMatrix[3][1] = mat.matrix[3][1];

	transformData.transformMatrix[0][2] = mat.matrix[0][2];
	transformData.transformMatrix[1][2] = mat.matrix[1][2];
	transformData.transformMatrix[2][2] = mat.matrix[2][2];
	transformData.transformMatrix[3][2] = mat.matrix[3][2];

	transformData.transformMatrix[0][3] = mat.matrix[0][3];
	transformData.transformMatrix[1][3] = mat.matrix[1][3];
	transformData.transformMatrix[2][3] = mat.matrix[2][3];
	transformData.transformMatrix[3][3] = mat.matrix[3][3];

	int parentCount = transformer.parentCount();
	for (int i = 0; i < parentCount; ++i)
	{
		MObject parentObject(transformer.parent(i));
		if (parentObject.apiType() == MFn::kWorld)
		{
			transformData.parentNames.push_back("World");
		}
		else if (parentObject.apiType() == MFn::kMesh)
		{
			MFnMesh parent(parentObject);
			transformData.parentNames.push_back(parent.name().asChar());
		}
		else if (parentObject.apiType() == MFn::kTransform)
		{
			MFnTransform parent(parentObject);
			transformData.parentNames.push_back(parent.name().asChar());
		}
	}

	//comlib.send(&transformData, ComLib::MSG_TYPE::ALLOCATETRANSFORMER, sizeof(ComLib::TransformerData));
}

//Support will be added.
//void attributeAddedRemovedCallback(MNodeMessage::AttributeMessage msg, MPlug& plug, void* clientData)
//{
//	if (msg & MNodeMessage::AttributeMessage::kAttributeAdded ||
//		msg & MNodeMessage::AttributeMessage::kAttributeRemoved)
//	{
//		MGlobal::displayInfo("test +-");
//	}
//	else if(msg & MNodeMessage::AttributeMessage::kAttributeArrayAdded ||
//			msg & MNodeMessage::AttributeMessage::kAttributeArrayRemoved)
//	{
//		MGlobal::displayInfo("test []+-");
//	}
//}
void attributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void*clientData)
{
	MStatus res;
	MString debugString;

	if (msg & MNodeMessage::AttributeMessage::kAttributeEval ||
		msg & MNodeMessage::AttributeMessage::kAttributeSet)
	{
		MGlobal::displayInfo("************************");

		MFnDependencyNode attributeOwnerNode(plug.node());
		MString ownerName(attributeOwnerNode.name());
		MFn::Type ownerType(plug.node().apiType());
		MGlobal::displayInfo(ownerName);
		MGlobal::displayInfo(attributeOwnerNode.typeName());

		MFnAttribute attribute(plug);
		MString attributeName(attribute.name().asChar());
		MGlobal::displayInfo(attributeName);

		//for (size_t i = 0; i < attributeOwnerNode.attributeCount(); i++)
		//{
		//	MFnAttribute att(attributeOwnerNode.attribute(i));
		//	debugString = att.name();
		//	MGlobal::displayInfo(debugString);
		//}
		
		//Switch ordered after frequent access order
		switch (ownerType)
		{
			//Transform options
			case MFn::kTransform:
				if (attributeName == "translate")
				{
					MFnTransform transform(plug.node());
					getTransfromData(transform);
				}

			//Camera options
			case MFn::kCamera:
				if (attributeName == "centerOfInterest")
				{
					MFnCamera cam(plug.node());
					getProjectionMatrix(cam);
				}
				break;
			//Mesh Options
			case MFn::kMesh:
				if (attributeName == "pnts")
				{
					MGlobal::displayInfo("fetch moved pnts");
					//
					// <---- send pnts data
					//
					MFnMesh mesh(plug.node());
					sendPoints(mesh);
				}
				break;
			case MFn::kPolyTweakUV:
				if (attributeName == "output")
				{
					MGlobal::displayInfo("ass");
				}
				break;

			case MFn::kPolySplit:
			case MFn::kPolySplitVert:
			case MFn::kPolySplitEdge:
			case MFn::kPolyTweak:
			case MFn::kPolyExtrudeVertex:
			case MFn::kPolyExtrudeEdge:
			case MFn::kPolyExtrudeFacet:
			case MFn::kPolyMoveVertex:
			case MFn::kPolyBevel:
			case MFn::kPolyBevel2:
			case MFn::kPolyBevel3:
			case MFn::kPolyReduce:
			case MFn::kPolySmooth:
			case MFn::kPolySmoothFacet:
			case MFn::kDeleteComponent:
			case MFn::kPolyBridgeEdge:
			case MFn::kPolyMergeVert:
			case MFn::kPolyMergeFacet:
			case MFn::kPolyMergeEdge:
			case MFn::kPolyMirror:
				if (attributeName == "output")
				{
					MFnMesh mesh(plug.node());
					getTotalVertexData(mesh);
				}
				break;
			case MFn::kFileTexture:
				if (attributeName == "fileTextureName")
				{
					MGlobal::displayInfo("major Success!!");
					if (res == MS::kSuccess)
					{
						MString texturePath;
						plug.getValue(texturePath);
						MGlobal::displayInfo(texturePath);

						//
						//<----- Add to meshData
						//
					}
				}
				break;
			//Light options
			case MFn::kPointLight:
				break;

			//case MFn::kShadingEngine:
			//	if (attributeName == "surfaceShader")
			//	{
			//		MGlobal::displayInfo("surfaceShader call");
			//		MFnDagNode dag(attributeOwnerNode.object());
			//		MGlobal::displayInfo(attribute.name());
			//		debugString = attribute.type();
			//		MGlobal::displayInfo(debugString);
			//		int count = dag.parentCount();
			//		debugString = count;
			//		MGlobal::displayInfo(debugString);
			//		for (size_t i = 0; i < count; i++)
			//		{
			//			MObject parent(dag.parent(i));
			//			MFnDagNode parentDag(parent);
			//			MGlobal::displayInfo(parentDag.name());
			//		}

			//	}
			//	break;
			default:
				break;
		}

		

		//MGlobal::displayInfo("************************");
	}
	else if (msg & MNodeMessage::AttributeMessage::kConnectionMade)
	{
		MGlobal::displayInfo("************************");
		MFnDependencyNode attributeOwnerNode(plug.node());
		MString ownerName(attributeOwnerNode.name());
		MFn::Type ownerType(plug.node().apiType());
		MGlobal::displayInfo(ownerName);
		MGlobal::displayInfo(attributeOwnerNode.typeName());

		MFnAttribute attribute(plug);
		MString attributeName(attribute.name().asChar());
		MGlobal::displayInfo(attributeName);
		
		//Get shader object array (groups == shaders? Might be other objects in the array, hard-coded risky)
		if (ownerType == MFn::kMesh && attributeName == "instObjGroups")
		{
			MGlobal::displayInfo("enter nirvana");
			//
			//
			//
			MFnMesh mesh(plug.node());
			MObjectArray shaderArray;
			MIntArray faceIndices;
			mesh.getConnectedShaders(0, shaderArray, faceIndices);
			for (UINT i = 0; i < shaderArray.length(); ++i)
			{
				//initialShadingGroup
				MFnDependencyNode shaderGroup(shaderArray[i]);

				MPlug shaderPlug = shaderGroup.findPlug("surfaceShader", res);
				if (res
					== MS::kSuccess)
				{
					MPlugArray shaderConnections;
					shaderPlug.connectedTo(shaderConnections, true, false);
					for (UINT j = 0; j < shaderConnections.length(); ++j)
					{
						//Shader Material .outColor plug
						MPlug outColorPlug = shaderConnections[j];
						//Shader Material (Lambert/Blinn)
						MFnDependencyNode materialNode(outColorPlug.node());
						MGlobal::displayInfo("-1-");
						MGlobal::displayInfo(materialNode.name());
						MGlobal::displayInfo(materialNode.typeName());

						for (size_t k = 0; k < materialNode.attributeCount(); ++k)
						{
							MFnAttribute tempf = materialNode.attribute(k);
							MGlobal::displayInfo(tempf.name());
						}

						// Fetch the plug data: 
						//*material*.color
						MPlug colorPlug = materialNode.findPlug("color", res);
						if (colorPlug.node().apiType() == MFn::kData3Float)
						{
							MDataHandle dataHandle = {};
							colorPlug.getValue(dataHandle);
							MFloatVector color = dataHandle.asFloatVector();
							debugString = "R: ";
							debugString += color.x;
							debugString += "G: ";
							debugString += color.y;
							debugString += "B: ";
							debugString += color.z;
							MGlobal::displayInfo(debugString);
							//
							//<----- Add to meshData
							//
						}

						//*material*.ambientColor
						MPlug ambientColorPlug = materialNode.findPlug("ambientColor", res);
						MGlobal::displayInfo("-2-");
						if (ambientColorPlug.node().apiType() == MFn::kData3Float)
						{
							MDataHandle dataHandle = {};
							ambientColorPlug.getValue(dataHandle);
							MFloatVector color = dataHandle.asFloatVector();
							debugString = "R: ";
							debugString += color.x;
							debugString += "G: ";
							debugString += color.y;
							debugString += "B: ";
							debugString += color.z;
							MGlobal::displayInfo(debugString);
							//
							//<----- Add to meshData
							//
						}

						//*material*.transparency
						MPlug transparencyPlug = materialNode.findPlug("transparency", res);
						MGlobal::displayInfo("-3-");
						if (transparencyPlug.node().apiType() == MFn::kData3Float)
						{
							MDataHandle dataHandle = {};
							transparencyPlug.getValue(dataHandle);
							MFloatVector color = dataHandle.asFloatVector();
							debugString = "R: ";
							debugString += color.x;
							debugString += "G: ";
							debugString += color.y;
							debugString += "B: ";
							debugString += color.z;
							MGlobal::displayInfo(debugString);
							//
							//<----- Add to meshData
							//
						}

						//Get connected textures
						MItDependencyGraph dgIt(
							colorPlug,
							MFn::kFileTexture,
							MItDependencyGraph::kUpstream,
							MItDependencyGraph::kBreadthFirst,
							MItDependencyGraph::kNodeLevel,
							&res
						);
						dgIt.disablePruningOnFilter();

						if (!dgIt.isDone())
						{
							MObject textureObject = dgIt.currentItem();
							//Image Name: (plug)
							MPlug fileNamePlug = MFnDependencyNode(textureObject).findPlug("fileTextureName", res);
							if (res == MS::kSuccess)
							{
								MString texturePath;
								fileNamePlug.getValue(texturePath);
								MGlobal::displayInfo(texturePath);

								//
								//<----- Add to meshData
								//
							}
						}

					}
				}
			}
		}
		
		MGlobal::displayInfo("************************");

		
	}

	//if (msg & MNodeMessage::AttributeMessage::kAttributeSet
	//	|| msg & MNodeMessage::AttributeMessage::kAttributeEval)
	//{
	//	if (obj.apiType() == MFn::kMesh)
	//	{
	//		MFnMesh mesh(obj);
	//		outputMeshData(mesh);
	//	}
	//	else if (obj.apiType() == MFn::kTransform)
	//	{
	//		MFnTransform transformer(obj);
	//		getTransfromData(transformer);
	//	}
	//}
}


// NODE CREATION
//-----------------------------------------------------------
//Uncomment ComLib
void createNewMayaCamera(MFnCamera& cam, MFnTransform& camTransform)
{
	ComLib::CameraData camData;
	MObject camObject(cam.object());
	MObject transformerObject(camTransform.object());
	MStatus res;

	//Query cam name and projection matrix
	MString camUuid = cam.uuid();
	MString camName = cam.name();
	MFloatMatrix projectionMat = cam.projectionMatrix();

	//Query view matrix and name
	MString camTransformerUuid = camTransform.uuid();
	MString camTransformerName = camTransform.name();
	MFloatMatrix viewMatrix = camTransform.transformation().asMatrix().matrix;

	//Query micro data (if needed, should be contained within the martix)
	double ARO = cam.aspectRatio();
	double HFOV = cam.horizontalFieldOfView();
	double VFOV = cam.verticalFieldOfView();
	double nPlane = cam.nearClippingPlane();
	double fPlane = cam.farClippingPlane();

	//Add Camera Callbacks
	MCallbackId NameChangedID = MNodeMessage::addNameChangedCallback(
		camObject,
		nameChangeCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(NameChangedID) == MS::kSuccess) {};
	}

	MCallbackId AttributeChangedID = MNodeMessage::addAttributeChangedCallback(
		camObject,
		attributeChangedCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(AttributeChangedID) == MS::kSuccess) {};
	}

	//Add Transformer Callbacks
	NameChangedID = MNodeMessage::addNameChangedCallback(
		transformerObject,
		nameChangeCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(NameChangedID) == MS::kSuccess) {};
	}

	AttributeChangedID = MNodeMessage::addAttributeChangedCallback(
		transformerObject,
		attributeChangedCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(AttributeChangedID) == MS::kSuccess) {};
	}

	int parentCount = cam.parentCount();
	for (size_t i = 0; i < parentCount; ++i)
	{
		MFnTransform camTransform(cam.parent(i));
		MString camTransformerName = camTransform.name();
		MString camTransformerUuid = camTransform.uuid();
		camData.camTransformerUuids.push_back(camTransformerUuid.asChar());
		camData.camTransformerNames.push_back(camTransformerName.asChar());
	}

	// Fill the data for transfer to engine.
	camData.uuid = camUuid.asChar();
	camData.camName = camName.asChar();

	//ProjMat
	camData.projMat[0][0] = projectionMat.matrix[0][0];
	camData.projMat[1][0] = projectionMat.matrix[1][0];
	camData.projMat[2][0] = projectionMat.matrix[2][0];
	camData.projMat[3][0] = projectionMat.matrix[3][0];

	camData.projMat[0][1] = projectionMat.matrix[0][1];
	camData.projMat[1][1] = projectionMat.matrix[1][1];
	camData.projMat[2][1] = projectionMat.matrix[2][1];
	camData.projMat[3][1] = projectionMat.matrix[3][1];

	camData.projMat[0][2] = projectionMat.matrix[0][2];
	camData.projMat[1][2] = projectionMat.matrix[1][2];
	camData.projMat[2][2] = projectionMat.matrix[2][2];
	camData.projMat[3][2] = projectionMat.matrix[3][2];

	camData.projMat[0][3] = projectionMat.matrix[0][3];
	camData.projMat[1][3] = projectionMat.matrix[1][3];
	camData.projMat[2][3] = projectionMat.matrix[2][3];
	camData.projMat[3][3] = projectionMat.matrix[3][3];

	//Other Data
	camData.ARO = ARO;
	camData.HFOV = HFOV;
	camData.VFOV = VFOV;
	camData.nPlane = nPlane;
	camData.fPlane = fPlane;

	//comlib.send(&camData, ComLib::MSG_TYPE::ALLOCATECAMERA, sizeof(ComLib::CameraData));

	ComLib::TransformerData transformData;
	transformData.transformerName = camTransformerName.asChar();
	transformData.transformerUuid = camTransformerUuid.asChar();
	transformData.transformMatrix[0][0] = viewMatrix.matrix[0][0];
	transformData.transformMatrix[1][0] = viewMatrix.matrix[1][0];
	transformData.transformMatrix[2][0] = viewMatrix.matrix[2][0];
	transformData.transformMatrix[3][0] = viewMatrix.matrix[3][0];

	transformData.transformMatrix[0][1] = viewMatrix.matrix[0][1];
	transformData.transformMatrix[1][1] = viewMatrix.matrix[1][1];
	transformData.transformMatrix[2][1] = viewMatrix.matrix[2][1];
	transformData.transformMatrix[3][1] = viewMatrix.matrix[3][1];

	transformData.transformMatrix[0][2] = viewMatrix.matrix[0][2];
	transformData.transformMatrix[1][2] = viewMatrix.matrix[1][2];
	transformData.transformMatrix[2][2] = viewMatrix.matrix[2][2];
	transformData.transformMatrix[3][2] = viewMatrix.matrix[3][2];

	transformData.transformMatrix[0][3] = viewMatrix.matrix[0][3];
	transformData.transformMatrix[1][3] = viewMatrix.matrix[1][3];
	transformData.transformMatrix[2][3] = viewMatrix.matrix[2][3];
	transformData.transformMatrix[3][3] = viewMatrix.matrix[3][3];

	//comlib.send(&transformData, ComLib::MSG_TYPE::ALLOCATETRANSFORMER, sizeof(ComLib::TransformerData));

	MGlobal::displayInfo(MString("CameraName: " + camName));
	
	MGlobal::displayInfo(MString("Cam:") + viewMatrix.matrix[0][0] + " " + viewMatrix.matrix[0][1] + " " + viewMatrix.matrix[0][2] + " " + viewMatrix.matrix[0][3]);
	MGlobal::displayInfo(MString("Cam:") + viewMatrix.matrix[1][0] + " " + viewMatrix.matrix[1][1] + " " + viewMatrix.matrix[1][2] + " " + viewMatrix.matrix[1][3]);
	MGlobal::displayInfo(MString("Cam:") + viewMatrix.matrix[2][0] + " " + viewMatrix.matrix[2][1] + " " + viewMatrix.matrix[2][2] + " " + viewMatrix.matrix[2][3]);
	MGlobal::displayInfo(MString("Cam:") + viewMatrix.matrix[3][0] + " " + viewMatrix.matrix[3][1] + " " + viewMatrix.matrix[3][2] + " " + viewMatrix.matrix[3][3]);
	
	MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[0][0] + " " + projectionMat.matrix[0][1] + " " + projectionMat.matrix[0][2] + " " + projectionMat.matrix[0][3]);
	MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[1][0] + " " + projectionMat.matrix[1][1] + " " + projectionMat.matrix[1][2] + " " + projectionMat.matrix[1][3]);
	MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[2][0] + " " + projectionMat.matrix[2][1] + " " + projectionMat.matrix[2][2] + " " + projectionMat.matrix[2][3]);
	MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[3][0] + " " + projectionMat.matrix[3][1] + " " + projectionMat.matrix[3][2] + " " + projectionMat.matrix[3][3]);
}

void nodeCreationCallback(MObject& object, void* clientData)
{
	MStatus res;


	if (object.apiType() == MFn::kMesh)
	{
		MString nodeName;
		MFnDagNode dag(object);

		nodeName = "Mesh node name: ";
		nodeName += dag.name();
		MGlobal::displayInfo(nodeName);
		
		// Allocate a space since mesh node is not calculated
		//
		//<----- Allocate object in engine
		//

	}
	else if (object.apiType() == MFn::kTransform)
	{
		MString nodeName;
		MFnTransform trans(object);

		//Create Empty Node in engine, reserve name
		nodeName = "Transform node name: ";
		nodeName += trans.name();
		MGlobal::displayInfo(nodeName);

		//Fill with existing information
		getTransfromData(trans);
	}

	MCallbackId NameChangedID = MNodeMessage::addNameChangedCallback(
		object,
		nameChangeCallback,
		&clientData,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(NameChangedID) == MS::kSuccess) {};
	}

	//MCallbackId AttributeAddedRemovedID = MNodeMessage::addAttributeAddedOrRemovedCallback(
	//	object,
	//	attributeAddedRemovedCallback,
	//	&clientData,
	//	&res);
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(AttributeAddedRemovedID) == MS::kSuccess) {};
	//}

	MCallbackId AttributeChangedID = MNodeMessage::addAttributeChangedCallback(
		object,
		attributeChangedCallback,
		&clientData,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(AttributeChangedID) == MS::kSuccess) {};
	}

}
void nodeDeleteCallback(MObject& object, void* clientData)
{
	if (object.apiType() == MFn::kMesh)
	{
		MFnDependencyNode meshNode(object);

		MGlobal::displayInfo(meshNode.absoluteName() + " is deleted");

		//MFnMesh mesh(object);
		//MGlobal::displayInfo(mesh.absoluteName() + " is deleted");
	}
	else if (object.apiType() == MFn::kTransform)
	{
	}
	else if (object.apiType() == MFn::kMesh)
	{
	}
}

void createAddCallbackChildNode(MObject currentObject)
{
	MStatus res;
	MString debugString;

	MFnDagNode currentDAG(currentObject);

	for (size_t i = 0; i < currentDAG.childCount(); i++)
	{
		MObject childObject(currentDAG.child(i));
		MFnDagNode childDAG(childObject);
		MString childUuid = childDAG.uuid();
		MString childName = childDAG.name();
		MString childType = childDAG.typeName();
		MGlobal::displayInfo(childType);
		
		MCallbackId NameChangedID = MNodeMessage::addNameChangedCallback(
			childObject,
			nameChangeCallback,
			NULL,
			&res);
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(NameChangedID) == MS::kSuccess) {};
		}

		MCallbackId AttributeChangedID = MNodeMessage::addAttributeChangedCallback(
			childObject,
			attributeChangedCallback,
			NULL,
			&res);
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(AttributeChangedID) == MS::kSuccess) {};
		}

		//Checks if the children are of supported type and of interest.
		if (childObject.apiType() == MFn::kTransform)
		{
			
		}
		if (childObject.apiType() == MFn::kMesh)
		{
			//Node is empty until node update.
			//Only possibility to add callbacks.
			//clientData == empty | clientData == User-defined data? (bad practice? no other way?)

			// + parent Name
			//
			//<----- Allocate object in engine
			//

		}
		else if (childObject.apiType() == MFn::kCamera)
		{
			//Add camera
		}
		else if (childObject.apiType() == MFn::kPointLight)
		{
			//Add light
		}

		if (childDAG.childCount() > 0) 
		{
			createAddCallbackChildNode(childObject);
		}

	}
}

//Needs finish
void getExistingScene()
{
	MString debugString;
	MStatus res;

	//Queries the current viewport.
	M3dView sceneView;
	sceneView = sceneView.active3dView();

	//Queries the current cameras DagPath.
	MDagPath camDagPath;
	sceneView.getCamera(camDagPath);

	//Queries the camera & projection matrix.
	MFnCamera cam(camDagPath.node());

	//Fetches the parent of the camera which will always be its transformer aka. view matrix.
	MFnTransform camTransform = cam.parent(0);

	createNewMayaCamera(cam, camTransform);

	//Query root node
	MFnDagNode rootDAG(camTransform.parent(0));
	MGlobal::displayInfo("RootName:");
	MGlobal::displayInfo(rootDAG.name());
	MGlobal::displayInfo(rootDAG.typeName());
	MGlobal::displayInfo("********************************");

	MFnDependencyNode som(rootDAG.object());
	MGlobal::displayInfo("Root Node:");
	MGlobal::displayInfo(rootDAG.name());
	MGlobal::displayInfo(rootDAG.typeName());
	MGlobal::displayInfo("********************************");

	//Look at existing objects in the scene.
	for (size_t i = 0; i < rootDAG.childCount(); i++)
	{
		//Will probably be a transform, if statement to ensure not to stumble upon unexpected results.
		//Since we want to find the transformer for our objects anyway. (Typename hardcoded, possible fix?)
		MFnDagNode rootChildDAG(rootDAG.child(i));
		if (rootChildDAG.typeName() == "transform")
		{
			MObject transformObject(rootChildDAG.object());
			MFnTransform transform(transformObject);
			MString currentTransformName(transform.name());

			//Hard-coded. Possible automation
			//Essencial scene objects, carefull when modifying
			if (currentTransformName != "groundPlane_transform"
				|| currentTransformName != "defaultUfeProxyParent"
				|| currentTransformName != "persp"
				|| currentTransformName != "top"
				|| currentTransformName != "front"
				|| currentTransformName != "side")
			{
				//Again, first nodes accessable as children is transform.
				getTransfromData(transform);

				MCallbackId NameChangedID = MNodeMessage::addNameChangedCallback(
					transformObject,
					nameChangeCallback,
					NULL,
					&res);
				if (res == MS::kSuccess)
				{
					if (myCallbackArray.append(NameChangedID) == MS::kSuccess) {};
				}

				MCallbackId AttributeChangedID = MNodeMessage::addAttributeChangedCallback(
					transformObject,
					attributeChangedCallback,
					NULL,
					&res);
				if (res == MS::kSuccess)
				{
					if (myCallbackArray.append(AttributeChangedID) == MS::kSuccess) {};
				}
			}

			if (transform.childCount() > 0)
			{
				createAddCallbackChildNode(transformObject);
			}
		}
	}
}

//PLUGIN
//-----------------------------------------------------------
//node shapes does not get the data until pulse? Nodes will be empty on existing and node creation.
//shapes get data at attribute changed.
static void undoCallback(void *clientData)
{
	MString info = "undoCallback : clientData = ";
	if (clientData)
		info += (unsigned)(MUintPtrSz)clientData;
	else
		info += "NULL";
	//MGlobal::displayInfo(info);
}
EXPORT MStatus initializePlugin(MObject obj)
{
	MStatus res = MS::kSuccess;
	MString debugString;

	MFnPlugin myPlugin(obj, "Maya plugin", "1.0", "Any", &res);
	if (MFAIL(res))
	{
		CHECK_MSTATUS(res);
		return res;
	}
	else
	{
		MGlobal::displayInfo("Maya plugin loaded!");
	}
	
	//Query all the existing data in the scene
	getExistingScene();

	MCallbackId nodeAddedId = MDGMessage::addNodeAddedCallback(
		nodeCreationCallback,
		kDefaultNodeType,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(nodeAddedId) == MS::kSuccess) {};
	}

	//MCallbackId nodeRemovedId = MDGMessage::addNodeRemovedCallback(
	//	nodeDeleteCallback,
	//	kDefaultNodeType,
	//	NULL,
	//	&res);
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(nodeRemovedId) == MS::kSuccess) {};
	//}
	
	//MCallbackId undoId = MEventMessage::addEventCallback(
	//	"Undo",
	//	undoCallback,
	//	NULL,
	//	&res);
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(undoId) == MS::kSuccess) {};
	//}
	//
	//MCallbackId camTranslateId = MUiMessage::add3dViewPostRenderMsgCallback(
	//	"modelPanel4",
	//	renderChangeCallback,
	//	NULL,
	//	&res
	//);

	return res;
}
EXPORT MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	MMessage::removeCallbacks(myCallbackArray);
	MGlobal::displayInfo("Maya plugin unloaded!");

	return MS::kSuccess;
}