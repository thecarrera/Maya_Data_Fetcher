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
// #	DAG nodes can be seen in hypergraph heirarchy, DG nodes in hypergraph connections					 #
// #																										 #
// #	Plugs values can be accessed through getValue(), numChildren() for loop{ MPLug }, or MFnSet			 #
// #	Names to use for finding plugs/attributes can be found through function 1.1							 #
// #																										 #
// ###########################################################################################################

// Hindsight notes:
//	* Plugs data is obtainable through DataHandles/DataBlocks
//	* Parenting only happens to transformers. Two shapes can't share the same parent?
//
//  * Separate further! Parenting | micro data. Send smaller amount of data!
//  * Optimize: lower data types, shared file size, algorithms.
//  * Renderer is shader driven https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=__developer_Maya_SDK_MERGED_Viewport_2_0_API_Maya_Viewport_2_0_API_Guide_Rendering_Framework_Shader_Driven_Update_html
//    Quering all materials will hang the application. Search by MFn::kLambert etc. or MFn::kShaderEngines
//    Check MFn::kAdskMaterial, ::kDiffuseMaterial, ::kMaterial (StingrayPBS) (Handle custom materials.)
//  * Can't find base color in StingrayPBR material in either attributes or plugs. (See how custom HardwareShaders work).
//  * Study push-pull system more, hard knowing when data is available.
//	* Maya crashes after loading then unloading plugin, memory leak etc or problem with Maya?
//  * Check if memcpy() can be replaced with reinterpret_cast<>, send ownership to receiver, 
//	  or implement signal system for when memory usage is finished and waiting for release.
//	
// * Combine callback in a queue, send buffer with combined commands each render call. Change comlib send to comlib.recordCommand(command).
//
// DOES NOT SUPPORT!!!
// DOES NOT SUPPORT!!!
// DOES NOT SUPPORT!!!
// DOES NOT SUPPORT!!!
// * REDO/UNDO
// * COMBINE
// * DELETE HISTORY
//
// ######### Functions ##############
// @@@ Function 1.1 @@@ Find all nodes attribute names @@@
//for (size_t i = 0; i < node.attributeCount(); i++)
//{
//	MFnAttribute attr(node.attribute(i));
//	debugString = i;
//	debugString += ": ";
//	debugString += attr.name();
//	MGlobal::displayInfo(debugString);
//}
// @@@ Function 2.2 @@@ 
//void traversePaths(MObject& object, int iteration)
//{
//	MString debugString = "";
//	MStatus res;
//
//	MFnDagNode node(object);
//	MDagPath currentDag(node.dagPath());
//	for (size_t i = 0; i < iteration; i++)
//	{
//		debugString += "-";
//	}
//	debugString += iteration;
//	debugString += ": ";
//	debugString += currentDag.fullPathName();
//	
//	MGlobal::displayInfo("***********************************");
//	int count = currentDag.childCount();
//	MGlobal::displayInfo("Count: " + count);
//	for (size_t i = 0; i < count; ++i)
//	{
//		MObject chObj(currentDag.node());
//		MFnDagNode chNode(chObj);
//		MDagPath chPath(chNode.dagPath());
//
//		if (chPath.childCount() > 0)
//		{
//			traversePaths(chObj, iteration + 1);
//		}
//		else
//		{
//			for (size_t i = 0; i < iteration + 1; i++)
//			{
//				debugString += "-";
//			}
//			debugString += iteration + 1;
//			debugString += ": ";
//			debugString += chPath.fullPathName();
//		}
//
//	}
//	MGlobal::displayInfo("***********************************");
//}

#include "maya_includes.h"
#include "ComLib.h"
#include <array>

MCallbackIdArray myCallbackArray;
ComLib comlib("sharedFileMap", (25ULL << 23ULL)); //200MB
ComLib connectionStatus("connection", (200ULL << 12ULL)); //100Kb

//###### OLD! ######
//void triangulateList(MIntArray Count, MIntArray& List)
//{
//	MIntArray newList;
//	int faceLeftOvers = 0;
//
//	for (UINT i = 0; i < Count.length(); ++i)
//	{
//		size_t facePointCount = Count[i];
//		faceLeftOvers = Count[i] % 4;
//
//		if (facePointCount < 3)
//		{
//			// Come up with error plan
//		}
//		else 
//		{
//			for (UINT j = 1; j <= facePointCount / 4; ++j)
//			{
//				UINT index = j * 4;
//				newList.append(List[index - 4]);
//				newList.append(List[index - 3]);
//				newList.append(List[index - 2]);
//
//				newList.append(List[index - 2]);
//				newList.append(List[index - 3]);
//				newList.append(List[index - 1]);
//			}
//
//			if (faceLeftOvers > 0)
//			{
//				newList.append(List[Count[i] - 3]);
//				newList.append(List[Count[i] - 2]);
//				newList.append(List[Count[i] - 1]);
//			}
//
//			List.clear();
//			List = newList;
//		}
//	}
//}
//##################
 

//General Micro Data
void pSendActiveCamera(MFnDagNode& camDAG)
{
	std::string uuid = camDAG.uuid().asString().asChar();
	size_t uuidSize = uuid.size();
	//ComPtr<char> msg {};
	//size_t messageSize = 0;
	//
	//memcpy(msg, &uuidSize, STSIZE);
	//messageSize += STSIZE;
	//memcpy(msg + messageSize, &uuid[0], uuidSize);
	//messageSize += uuidSize;
	//
	//comlib.send(&msg, ComLib::MSG_TYPE::ACTIVECAM, ComLib::ATTRIBUTE_TYPE::NONE, messageSize);
}
void pSendPlugData(MPlug& plug, MString ownerUuid, ComLib::ATTRIBUTE_TYPE attribute)
{
	MStatus res {};
	MString debugString {};
	
	// Generic Plug collecter. Separate types so that missmatch between data types does not happen.
	if (plug.asMObject().apiType() == MFn::Type::kData3Float)
	{
		MDataHandle dh{};
		plug.getValue(dh);
		MFloatVector fv{ dh.asFloat3() };
		float container[3] { fv.x, fv.y, fv.z };

		std::string uuid{ ownerUuid.asChar() };
		size_t uuidSize{ uuid.size() };
		std::vector<char> msg {};
		size_t messageSize {};
		
		msg.resize(
			msg.capacity() +
			STSIZE +
			uuidSize + 
			sizeof(container)
		);
		memcpy(msg.data(), &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;
		memcpy(msg.data() + messageSize, &container, sizeof(container));
		messageSize += sizeof(container);

		comlib.send(
			msg.data(), 
			ComLib::MSG_TYPE::UPDATEVALUES, 
			attribute, 
			messageSize
		);
	}
	else if (plug.asMObject().apiType() == MFn::Type::kMatrixData)
	{
		MDataHandle dh{};
		plug.getValue(dh);
		MMatrix mat{ dh.asMatrix() };
		double matrix[4][4] {
			mat.matrix[0][0],
			mat.matrix[1][0],
			mat.matrix[2][0],
			mat.matrix[3][0],

			mat.matrix[0][1],
			mat.matrix[1][1],
			mat.matrix[2][1],
			mat.matrix[3][1],

			mat.matrix[0][2],
			mat.matrix[1][2],
			mat.matrix[2][2],
			mat.matrix[3][2],

			mat.matrix[0][3],
			mat.matrix[1][3],
			mat.matrix[2][3],
			mat.matrix[3][3]
		};

		size_t matrixSize{ sizeof(matrix) };
		std::string uuid{ ownerUuid.asChar() };
		size_t uuidSize{ uuid.size() };
		std::vector<char> msg{};
		size_t messageSize {};

		msg.resize(
			msg.capacity() +
			STSIZE + 
			uuidSize +
			matrixSize
		);
		memcpy(msg.data(), &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;
		memcpy(msg.data() + messageSize, &matrix, matrixSize);
		messageSize += matrixSize;

		comlib.send(
			msg.data(),
			ComLib::MSG_TYPE::UPDATEVALUES, 
			ComLib::ATTRIBUTE_TYPE::MATRIX, 
			messageSize
		);
	}
}
void pSendPlugConnections(MPlug& plug, MString ownerName, MString ownerUuid, std::string messageType)
{
	MPlugArray connectedPlugs;
	plug.connectedTo(connectedPlugs, true, false);
	for (UINT i = 0; connectedPlugs.length(); ++i)
	{
		MFnDagNode connectedNode(connectedPlugs[i].node());
		//MGlobal::displayInfo(connectedNode.name());
	}
}

//Transform Micro Data
void pPrintMatrix(double mat[4][4])
{
	MString debugString {};
	debugString = mat[0][0];
	debugString += " ";
	debugString += mat[1][0];
	debugString += " ";
	debugString += mat[2][0];
	debugString += " ";
	debugString += mat[3][0];
	debugString += "\n";
	
	debugString += mat[0][1];
	debugString += " ";
	debugString += mat[1][1];
	debugString += " ";
	debugString += mat[2][1];
	debugString += " ";
	debugString += mat[3][1];
	debugString += "\n";
	
	debugString += mat[0][2];
	debugString += " ";
	debugString += mat[1][2];
	debugString += " ";
	debugString += mat[2][2];
	debugString += " ";
	debugString += mat[3][2];
	debugString += "\n";
	
	debugString += mat[0][3];
	debugString += " ";
	debugString += mat[1][3];
	debugString += " ";
	debugString += mat[2][3];
	debugString += " ";
	debugString += mat[3][3];
	debugString += "\n";
	
	MGlobal::displayInfo(debugString);
}
void pSendMatrixData(MObject& object)
{
	MString debugString;
	MStatus res;
	MFnTransform transform(object);
	std::string uuid = transform.uuid().asString().asChar();
	size_t uuidSize = uuid.size();

	MMatrix mat(transform.transformationMatrix());
	double matrix[4][4] = {
		mat.matrix[0][0],
		mat.matrix[1][0],
		mat.matrix[2][0],
		mat.matrix[3][0],

		mat.matrix[0][1],
		mat.matrix[1][1],
		mat.matrix[2][1],
		mat.matrix[3][1],

		mat.matrix[0][2],
		mat.matrix[1][2],
		mat.matrix[2][2],
		mat.matrix[3][2],

		mat.matrix[0][3],
		mat.matrix[1][3],
		mat.matrix[2][3],
		mat.matrix[3][3]
	};
	size_t matrixSize = sizeof(matrix);

	//pPrintMatrix(matrix);

	std::vector<char> msg {};
	size_t messageSize{ 0 };
	
	msg.resize(
		STSIZE + 
		uuidSize +
		matrixSize
	);
	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;
	memcpy(msg.data() + messageSize, &matrix, matrixSize);
	messageSize += matrixSize;
	
	comlib.send(
		msg.data(), 
		ComLib::MSG_TYPE::UPDATEVALUES, 
		ComLib::ATTRIBUTE_TYPE::MATRIX, 
		messageSize);
}

//Camera Micro Data
void pSendProjectionMatrix(MObject& object)
{
	MStatus res {};
	MString debugString {};
	MFnCamera cam {object};
	std::string uuid {cam.uuid().asString().asChar()};
	size_t uuidSize {uuid.size()};

	MMatrix projectionMat {cam.projectionMatrix().matrix};
	double projMat[4][4] {
		cam.projectionMatrix().matrix[0][0],
		cam.projectionMatrix().matrix[1][0],
		cam.projectionMatrix().matrix[2][0],
		cam.projectionMatrix().matrix[3][0],

		cam.projectionMatrix().matrix[0][1],
		cam.projectionMatrix().matrix[1][1],
		cam.projectionMatrix().matrix[2][1],
		cam.projectionMatrix().matrix[3][1],

		cam.projectionMatrix().matrix[0][2],
		cam.projectionMatrix().matrix[1][2],
		cam.projectionMatrix().matrix[2][2],
		cam.projectionMatrix().matrix[3][2],

		cam.projectionMatrix().matrix[0][3],
		cam.projectionMatrix().matrix[1][3],
		cam.projectionMatrix().matrix[2][3],
		cam.projectionMatrix().matrix[3][3]
	};

	std::vector<char> msg {};
	size_t messageSize {};
	msg.resize(
		STSIZE +
		uuidSize +
		sizeof(projMat)
	);
	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;
	memcpy(msg.data() + messageSize, &projMat, sizeof(projMat));
	messageSize += sizeof(projMat);

	comlib.send(
		msg.data(), 
		ComLib::MSG_TYPE::UPDATEVALUES, 
		ComLib::ATTRIBUTE_TYPE::PROJMATRIX, 
		messageSize
	);
}
void pSendCamData(MObject& object)
{
	MFnCamera cam(object);
	MString camUuid = cam.uuid();
	MString camName = cam.name();

	double ARO = cam.aspectRatio();
	double HFOV = cam.horizontalFieldOfView();
	double VFOV = cam.verticalFieldOfView();
	double nPlane = cam.nearClippingPlane();
	double fPlane = cam.farClippingPlane();

	//comlib.send(&camData, ComLib::MSG::UPDATEVALUES, ComLib::MSG_TYPE::CAMDATA, sizeof(comlib::message::camData));
}

//Mesh Micro Data
//void pSendVertexData2(MObject& object)
//{
//	MStatus res;
//	MString debugString;
//	char* msg {new char()};
//	size_t messageSize = 0;
//	double container3[10000][3];
//	size_t container3Size = sizeof(container3);
//	int counter = -1;
//
//	MFnMesh mesh(object);
//	std::string uuid = mesh.uuid().asString().asChar();
//	size_t uuidSize = uuid.size();
//	memcpy(msg, &uuidSize, STSIZE);
//	messageSize += STSIZE;
//	memcpy(msg + messageSize, &uuid[0], uuidSize);
//	messageSize += uuidSize;
//
//	MFloatPointArray vertexList;
//	MIntArray triangleCount;
//	MIntArray triangleList;
//	mesh.getPoints(vertexList, MSpace::kObject);
//	mesh.getTriangles(triangleCount, triangleList);
//
//	MGlobal::displayInfo("####################### ENTERED!!!! #####################");
//
//	if (vertexList.length() <= 0)
//	{
//		return;
//	}
//
//	////Allocate new vertex list in dx11/12 if count has changed
//	size_t vertexCount = vertexList.length();
//	memcpy(msg + messageSize, &vertexCount, STSIZE);
//	messageSize = STSIZE;
//	comlib.send(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::VERTEX, messageSize);
//	delete msg;
//	msg = new char();
//	messageSize = 0;
//
//	for (size_t i = 0; i < vertexCount; ++i)
//	{
//		++counter;
//		container3[counter][0] = vertexList[i].x;
//		container3[counter][1] = vertexList[i].y;
//		container3[counter][2] = vertexList[i].z;
//
//		if (counter % 9999 == 0 && counter != 0)
//		{
//			memcpy(msg + messageSize, &uuidSize, STSIZE);
//			messageSize += STSIZE;
//			memcpy(msg + messageSize, &uuid[0], uuidSize);
//			messageSize += uuidSize;
//			
//			// Position at i - counter when recieving data.
//			memcpy(msg + messageSize, &i, STSIZE);
//			messageSize += STSIZE;
//			memcpy(msg + messageSize, &counter, sizeof(counter));
//			messageSize += sizeof(counter);
//			memcpy(msg + messageSize, container3, container3Size);
//			messageSize += container3Size;
//			
//			comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEX, messageSize);
//			delete msg; msg = new char();
//			messageSize = 0;
//			counter = -1;
//		}
//	}
//	if (counter >= 0)
//	{
//		memcpy(msg + messageSize, &uuidSize, STSIZE);
//		messageSize += STSIZE;
//		memcpy(msg + messageSize, &uuid[0], uuidSize);
//		messageSize += uuidSize;
//		
//		// Position at vertexCount - counter when receiving data
//		memcpy(msg + messageSize, &vertexCount, STSIZE);
//		messageSize += STSIZE;
//		memcpy(msg + messageSize, &counter, sizeof(counter));
//		messageSize += sizeof(counter);
//		memcpy(msg + messageSize, container3, container3Size);
//		messageSize += container3Size;
//		
//		comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEX, messageSize);
//		delete msg; msg = new char();
//		messageSize = 0;
//		counter = -1;
//	}
//
//	MGlobal::displayInfo("vertex done!");
//
//	MFloatVectorArray normals;
//	MIntArray normalCount;
//	MIntArray normalList;
//	size_t normalsCount = normals.length();
//	mesh.getNormals(normals, MSpace::kObject);
//	mesh.getNormalIds(normalCount, normalList);
//
//	memcpy(msg, &uuidSize, STSIZE);
//	messageSize += STSIZE;
//	memcpy(msg + messageSize, &uuid[0], uuidSize);
//	messageSize += uuidSize;
//	memcpy(msg + messageSize, &normalsCount, STSIZE);
//	messageSize += STSIZE;
//
//	comlib.send(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::NORMAL, messageSize);
//	delete msg; msg = new char();
//	messageSize = 0;
//
//	for (size_t i = 0; i < normalsCount; ++i)
//	{
//		counter++;
//		container3[counter][0] = normals[i].x;
//		container3[counter][1] = normals[i].y;
//		container3[counter][2] = normals[i].z;
//
//		if (counter % 9999 == 0 && counter != 0)
//		{
//			memcpy(msg, &uuidSize, STSIZE);
//			messageSize += STSIZE;
//			memcpy(msg + messageSize, &uuid[0], uuidSize);
//			messageSize += uuidSize;
//			
//			// Position at i - counter when recieving data.
//			memcpy(msg + messageSize, &i, STSIZE);
//			messageSize += STSIZE;
//			memcpy(msg + messageSize, &counter, sizeof(counter));
//			messageSize += sizeof(counter);
//			memcpy(msg + messageSize, container3, container3Size);
//			messageSize += container3Size;
//			
//			comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMAL, messageSize);
//			delete msg; msg = new char();
//			messageSize = 0;
//			counter = -1;
//		}
//	}
//	if (counter >= 0)
//	{
//		memcpy(msg, &uuidSize, STSIZE);
//		messageSize += STSIZE;
//		memcpy(msg + messageSize, &uuid[0], uuidSize);
//		messageSize += uuidSize;
//		
//		// Position at vertexCount - counter when receiving data
//		memcpy(msg + messageSize, &normalsCount, STSIZE);
//		messageSize += STSIZE;
//		memcpy(msg + messageSize, &counter, sizeof(counter));
//		messageSize += sizeof(counter);
//		memcpy(msg + messageSize, container3, container3Size);
//		messageSize += container3Size;
//		
//		comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMAL, messageSize);
//		delete msg; msg = new char();
//		messageSize = 0;
//		counter = -1;
//	}
//
//	MGlobal::displayInfo("normal done!");
//
//	MFloatArray Us;
//	MFloatArray Vs;
//	size_t container[10000];
//	double container2[10000][2];
//	size_t uvListSize = sizeof(container2);
//	size_t uvCount = 0;
//	MString uvSetName;
//	MStringArray uvSets;
//	mesh.getUVSetNames(uvSets);
//
//	if (!(uvSets.length() <= 0 || mesh.numUVs(uvSets[0]) <= 0))
//	{
//		memcpy(msg, &uuidSize, STSIZE);
//		messageSize += STSIZE;
//		memcpy(msg + messageSize, &uuid[0], uuidSize);
//		messageSize += uuidSize;
//		
//		size_t uvSetsCount = uvSets.length();
//		memcpy(msg + messageSize, &uvSetsCount, STSIZE);
//		messageSize += STSIZE;
//
//		comlib.send(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::UVSETS, messageSize);
//		delete msg; msg = new char();
//		messageSize = 0;
//
//		for (size_t i = 0; i < uvSetsCount; ++i)
//		{
//			memcpy(msg, &uuidSize, STSIZE);
//			messageSize += STSIZE;
//			memcpy(msg + messageSize, &uuid[0], uuidSize);
//			messageSize += uuidSize;
//
//			mesh.getUVs(Us, Vs, &uvSets[i]);
//			uvCount = Us.length();
//			memcpy(msg + messageSize, &uvCount, STSIZE);
//			messageSize += STSIZE;
//
//			memcpy(msg + messageSize, &i, STSIZE);
//			messageSize += STSIZE;
//
//			std::string uvSetName = uvSets[i].asChar();
//			size_t setNameLength = uvSetName.size();			
//			memcpy(msg + messageSize, &setNameLength, STSIZE);
//			messageSize += STSIZE;
//			memcpy(msg + messageSize, &uvSetName[0], setNameLength);
//			messageSize += setNameLength;
//			
//
//			comlib.send(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::UVSET, messageSize);
//			delete msg; msg = new char();
//			messageSize = 0;
//
//			for (size_t j = 0; j < uvCount; ++j)
//			{
//				counter++;
//				container2[counter][0] = Us[j];
//				container2[counter][1] = Vs[j];
//
//				if (counter % 9999 == 0 && counter != 0)
//				{
//					memcpy(msg, &uuidSize, STSIZE);
//					messageSize += STSIZE;
//					memcpy(msg + messageSize, &uuid[0], uuidSize);
//					messageSize += uuidSize;
//					
//					// Position at vertexCount - counter when receiving data
//					memcpy(msg + messageSize, &j, STSIZE);
//					messageSize += STSIZE;
//					memcpy(msg + messageSize, &counter, sizeof(counter));
//					messageSize += sizeof(counter);
//					memcpy(msg + messageSize, container2, sizeof(container2));
//					messageSize += sizeof(container2);
//						
//					memcpy(msg + messageSize, &i, STSIZE);
//					messageSize += STSIZE;
//					
//					comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UV, messageSize);
//					delete msg; msg = new char();
//					messageSize = 0;
//					counter = -1;
//				}
//			}
//			if (counter >= 0)
//			{
//				memcpy(msg, &uuidSize, STSIZE);
//				messageSize += STSIZE;
//				memcpy(msg + messageSize, &uuid[0], uuidSize);
//				messageSize += uuidSize;
//
//				// Position at vertexCount - counter when receiving data
//				memcpy(msg + messageSize, &uvCount, STSIZE);
//				messageSize += STSIZE;
//				memcpy(msg + messageSize, &counter, sizeof(counter));
//				messageSize += sizeof(counter);
//				memcpy(msg + messageSize, container2, sizeof(container2));
//				messageSize += sizeof(container2);
//
//				memcpy(msg + messageSize, &i, STSIZE);
//				messageSize += STSIZE;
//
//				comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UV, messageSize);
//				delete msg; msg = new char();
//				messageSize = 0;
//				counter = -1;
//			}
//
//			MItMeshPolygon faceIter(mesh.object());
//			MIntArray uvIDs;
//			
//			for (; !faceIter.isDone(); faceIter.next())
//			{
//				size_t faceVertexCount = faceIter.polygonVertexCount();
//				size_t leftOvers = faceVertexCount % 4;
//				MIntArray FaceUVIDs;
//				
//				for (int j = 0; j < faceVertexCount; ++j)
//				{
//					int uvID;
//					faceIter.getUVIndex(j, uvID, &uvSets[i]);
//					FaceUVIDs.append(uvID);
//				}
//
//				if (faceVertexCount < 3) 
//				{
//					// Come up with error plan
//				}
//				else
//				{
//					for (size_t j = 1; j <= faceVertexCount / 4; ++j)
//					{
//						size_t index = j * 4;
//						uvIDs.append(FaceUVIDs[index - 4]);
//						uvIDs.append(FaceUVIDs[index - 3]);
//						uvIDs.append(FaceUVIDs[index - 2]);
//
//						uvIDs.append(FaceUVIDs[index - 2]);
//						uvIDs.append(FaceUVIDs[index - 3]);
//						uvIDs.append(FaceUVIDs[index - 1]);
//					}
//
//					if (leftOvers > 0)
//					{
//						uvIDs.append(FaceUVIDs[faceVertexCount - 3]);
//						uvIDs.append(FaceUVIDs[faceVertexCount - 2]);
//						uvIDs.append(FaceUVIDs[faceVertexCount - 1]);
//					}
//				}
//
//
//
//				
//			}
//
//			size_t uvIDCount = uvIDs.length();
//			memcpy(msg, &uuidSize, STSIZE);
//			messageSize += STSIZE;
//			memcpy(msg + messageSize, &uuid[0], uuidSize);
//			messageSize += uuidSize;
//	
//			memcpy(msg + messageSize, &uvIDCount, STSIZE);
//			messageSize += STSIZE;
//
//			memcpy(msg + messageSize, &i, STSIZE);
//			messageSize += STSIZE;
//			
//			comlib.send(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::UVID, messageSize);
//			delete msg; msg = new char();
//			messageSize = 0;
//
//			for (size_t j = 0; j < uvIDCount; ++j)
//			{
//				counter++;
//				container[counter] = uvIDs[j];
//
//				if (counter % 9999 == 0 && counter != 0)
//				{
//	//				//memcpy(msg, &uuidSize, STSIZE);
//	//				//messageSize += STSIZE;
//	//				//memcpy(msg + messageSize, &uuid[0], uuidSize);
//	//				//messageSize += uuidSize;
//	
//	//				//memcpy(msg + messageSize, &setNameLength, STSIZE);
//	//				//messageSize += STSIZE;
//	//				//memcpy(msg + messageSize, &uvSetName[0], setNameLength);
//	//				//messageSize += setNameLength;
//
//	//				// Position at vertexCount - counter when receiving data
//	//				//memcpy(msg + messageSize, &j, STSIZE);
//	//				//messageSize += STSIZE;
//	//				//memcpy(msg + messageSize, &counter, sizeof(counter));
//	//				//messageSize += sizeof(counter);
//	//				//memcpy(msg + messageSize, container, sizeof(container));
//	//				//messageSize += sizeof(container);
//
//	//				//comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UVID, messageSize);
//	//				//msg.Reset();
//	//				//messageSize = 0;
//					counter = -1;
//				}
//			}
//			if (counter >= 0)
//			{
//	//			//memcpy(msg, &uuidSize, STSIZE);
//	//			//messageSize += STSIZE;
//	//			//memcpy(msg + messageSize, &uuid[0], uuidSize);
//	//			//messageSize += uuidSize;
//	
//	//			//memcpy(msg + messageSize, &setNameLength, STSIZE);
//	//			//messageSize += STSIZE;
//	//			//memcpy(msg + messageSize, &uvSetName[0], setNameLength);
//	//			//messageSize += setNameLength;
//
//	//			// Position at vertexCount - counter when receiving data
//	//			//memcpy(msg + messageSize, &uvIDCount, STSIZE);
//	//			//messageSize += STSIZE;
//	//			//memcpy(msg + messageSize, &counter, sizeof(counter));
//	//			//messageSize += sizeof(counter);
//	//			//memcpy(msg + messageSize, container, sizeof(container));
//	//			//messageSize += sizeof(container);
//
//	//			//comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UVID, messageSize);
//	//			//msg.Reset();
//	//			//messageSize = 0;
//				counter = -1;
//			}
//		}
//	}
//
//	MGlobal::displayInfo("uv done!");
//
//	if (triangleCount.length() != (triangleList.length() / 3))
//	{
//		triangulateList(normalCount, normalList);
//	}
//
//	MGlobal::displayInfo("triangulate done!");
//
//	size_t triangleListCount = triangleList.length();
//	size_t normalListCount = normalList.length();
//
//	////memcpy(msg, &uuidSize, STSIZE);
//	////messageSize += STSIZE;
//	////memcpy(msg + messageSize, &uuid[0], uuidSize);
//	////messageSize += uuidSize;
//	////memcpy(msg + messageSize, &triangleListCount, STSIZE);
//	////messageSize += STSIZE;
//
//	////comlib.send(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::VERTEXID, messageSize);
//	////msg.Reset();
//	////messageSize = 0;
//
//	////memcpy(msg, &uuidSize, STSIZE);
//	////messageSize += STSIZE;
//	////memcpy(msg + messageSize, &uuid[0], uuidSize);
//	////messageSize += uuidSize;
//	////memcpy(msg + messageSize, &normalListCount, STSIZE);
//	////messageSize += STSIZE;
//
//	////comlib.send(msg, ComLib::MSG_TYPE::ALLOCATE , ComLib::ATTRIBUTE_TYPE::NORMALID, messageSize);
//	////msg.Reset();
//	////messageSize = 0;
//
//	for (size_t i = 0; i < triangleList.length(); ++i)
//	{
//		counter++;
//		container[counter] = triangleList[i];
//
//		if (counter % 9999 == 0 && counter != 0)
//		{
//			//memcpy(msg, &uuidSize, STSIZE);
//			//messageSize += STSIZE;
//			//memcpy(msg + messageSize, &uuid[0], uuidSize);
//			//messageSize += uuidSize;
//
//			//memcpy(msg + messageSize, &i, STSIZE);
//			//messageSize += STSIZE;
//			//memcpy(msg + messageSize, &counter, sizeof(counter));
//			//messageSize += sizeof(counter);
//			//memcpy(msg + messageSize, container, sizeof(container));
//			//messageSize += sizeof(container);
//
//			//comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEXID, messageSize);
//			//msg.Reset();
//			//messageSize = 0;
//			counter = -1;
//		}
//	}
//	if (counter >= 0)
//	{
//		//memcpy(msg, &uuidSize, STSIZE);
//		//messageSize += STSIZE;
//		//memcpy(msg + messageSize, &uuid[0], uuidSize);
//		//messageSize += uuidSize;
//
//		//memcpy(msg + messageSize, &triangleListCount, STSIZE);
//		//messageSize += STSIZE;
//		//memcpy(msg + messageSize, &counter, sizeof(counter));
//		//messageSize += sizeof(counter);
//		//memcpy(msg + messageSize, container, sizeof(container));
//		//messageSize += sizeof(container);
//
//		//comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEXID, messageSize);
//		//msg.Reset();
//		//messageSize = 0;
//		counter = -1;
//	}
//
//	for (size_t i = 0; i < normalList.length(); ++i)
//	{
//		counter++;
//		container[counter] = normalList[i];
//
//		if (counter % 9999 == 0 && counter != 0)
//		{
//			//memcpy(msg, &uuidSize, STSIZE);
//			//messageSize += STSIZE;
//			//memcpy(msg + messageSize, &uuid[0], uuidSize);
//			//messageSize += uuidSize;
//
//			//memcpy(msg + messageSize, &i, STSIZE);
//			//messageSize += STSIZE;
//			//memcpy(msg + messageSize, &counter, sizeof(counter));
//			//messageSize += sizeof(counter);
//			//memcpy(msg + messageSize, container, sizeof(container));
//			//messageSize += sizeof(container);
//
//			//comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMALID, messageSize);
//			//msg.Reset();
//			//messageSize = 0;
//			counter = -1;
//		}
//	}
//	if (counter >= 0)
//	{
//		//memcpy(msg, &uuidSize, STSIZE);
//		//messageSize += STSIZE;
//		//memcpy(msg + messageSize, &uuid[0], uuidSize);
//		//messageSize += uuidSize;
//
//		//memcpy(msg + messageSize, &normalListCount, STSIZE);
//		//messageSize += STSIZE;
//		//memcpy(msg + messageSize, &counter, sizeof(counter));
//		//messageSize += sizeof(counter);
//		//memcpy(msg + messageSize, container, sizeof(container));
//		//messageSize += sizeof(container);
//
//		//comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMALID, messageSize);
//		//msg.Reset();
//		//messageSize = 0;
//		counter = -1;
//	}
//	MGlobal::displayInfo("IDs done!");
//}
void pSendVertexData(MObject& object)
{
	MStatus res {};
	MString debugString {};

	struct VERTEX
	{
		double point[3]		{};
		double normal[3]	{};
		double uv[2]		{};
	};
	std::vector<VERTEX> vertexList {};
	vertexList.resize(10000);

	MFnMesh mesh{ object };
	std::vector<char> msg {};
	size_t messageSize {};

	MStringArray uvSetNames{};
	mesh.getUVSetNames(uvSetNames);

	MItMeshVertex vertexIt{ object, &res };
	if (res == MS::kSuccess)
	{
		std::string uuid{mesh.uuid().asString().asChar()};
		size_t uuidSize{uuid.size()};

		msg.resize(
			STSIZE +
			uuidSize + 
			sizeof(int)
		);
		memcpy(msg.data(), &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;

		int vertexCount {vertexIt.count()};
		memcpy(msg.data() + messageSize, &vertexCount, sizeof(int));
		messageSize += sizeof(int);

		comlib.send(
			msg.data(), 
			ComLib::MSG_TYPE::ALLOCATE, 
			ComLib::ATTRIBUTE_TYPE::VERTEX, 
			messageSize
		);
		//DONE ##############################################################################################
		msg.clear();
		msg.resize(
			STSIZE +
			uuidSize +
			sizeof(int) * 2 +
			vertexList.size()
		);
		messageSize = 0;
		int counter {-1};

		for (; !vertexIt.isDone(); vertexIt.next())
		{
			++counter;
			int index{vertexIt.index()};
			
			MPoint point{vertexIt.position(MSpace::kObject)};
			
			vertexList[counter].point[0] = point.x;
			vertexList[counter].point[1] = point.y;
			vertexList[counter].point[2] = point.z;
		
			MVector normal{};
			vertexIt.getNormal(normal, MSpace::kObject);
			vertexList[counter].normal[0] = normal.x;
			vertexList[counter].normal[1] = normal.y;
			vertexList[counter].normal[2] = normal.z;
		
			float2 uv{};
			vertexIt.getUV(uv, &uvSetNames[0]);
			vertexList[counter].uv[0] = uv[0];
			vertexList[counter].uv[1] = uv[1];
		
			if (counter % 9999 == 0 && counter != 0)
			{
				memcpy(msg.data(), &uuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
				messageSize += uuidSize;
				
				memcpy(msg.data() + messageSize, &index, sizeof(int));
				messageSize += sizeof(int);
				memcpy(msg.data() + messageSize, &counter, sizeof(int));
				messageSize += sizeof(counter);
				memcpy(msg.data() + messageSize, &vertexList[0], vertexList.size());
				messageSize += vertexList.size();

				comlib.send(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::VERTEX, 
					messageSize
				);
				messageSize = 0;
				counter = -1;
			}
		}
		if (counter >= 0)
		{
			memcpy(msg.data(), &uuidSize, STSIZE);
			messageSize += STSIZE;
			memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
			messageSize += uuidSize;

			memcpy(msg.data() + messageSize, &vertexCount, sizeof(int));
			messageSize += sizeof(int);
			memcpy(msg.data() + messageSize, &counter, sizeof(int));
			messageSize += sizeof(int);
			memcpy(msg.data() + messageSize, &vertexList[0], vertexList.size());
			messageSize += vertexList.size();

			comlib.send(
				msg.data(), 
				ComLib::MSG_TYPE::UPDATEVALUES, 
				ComLib::ATTRIBUTE_TYPE::VERTEX, 
				messageSize
			);
			messageSize = 0;
			counter = -1;
		}
		
		std::vector<size_t> vertexIDs {};
		vertexIDs.resize(10000);
		MIntArray triangleCount	{};
		MIntArray triangleList	{};
		mesh.getTriangles(triangleCount, triangleList);
		unsigned int triangleListCount {triangleList.length()};
		
		msg.resize(
			STSIZE +
			uuidSize +
			sizeof(unsigned int)
		);
		memcpy(msg.data(), &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;
		
		memcpy(msg.data() + messageSize, &triangleListCount, sizeof(unsigned int));
		messageSize += sizeof(unsigned int);
		
		comlib.send(
			msg.data(),
			ComLib::MSG_TYPE::ALLOCATE,
			ComLib::ATTRIBUTE_TYPE::VERTEXID,
			messageSize
		);

		messageSize = 0;
		msg.resize(
			STSIZE +
			uuidSize +
			sizeof(unsigned int) +
			sizeof(int) +
			vertexIDs.size()
		);
		for (UINT i = 0; i < triangleList.length(); ++i)
		{
			counter++;
			vertexIDs[counter] = triangleList[i];
		
			if (counter % 9999 == 0 && counter != 0)
			{
				memcpy(msg.data(), &uuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
				messageSize += uuidSize;
		
				memcpy(msg.data() + messageSize, &i, sizeof(unsigned int));
				messageSize += sizeof(unsigned int);
				memcpy(msg.data() + messageSize, &counter, sizeof(int));
				messageSize += sizeof(int);
				memcpy(msg.data() + messageSize, &vertexIDs[0], vertexIDs.size());
				messageSize += vertexIDs.size();
		
				comlib.send(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::VERTEXID, 
					messageSize
				);
				messageSize = 0;
				counter = -1;
			}
		}
		if (counter >= 0)
		{
			memcpy(msg.data(), &uuidSize, STSIZE);
			messageSize += STSIZE;
			memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
			messageSize += uuidSize;
		
			memcpy(msg.data() + messageSize, &triangleListCount, sizeof(unsigned int));
			messageSize += sizeof(unsigned int);
			memcpy(msg.data() + messageSize, &counter, sizeof(int));
			messageSize += sizeof(int);
			memcpy(msg.data() + messageSize, &vertexIDs[0], vertexIDs.size());
			messageSize += vertexIDs.size();
		
			comlib.send(
				msg.data(),
				ComLib::MSG_TYPE::UPDATEVALUES,
				ComLib::ATTRIBUTE_TYPE::VERTEXID,
				messageSize
			);
		}
	}
	else
	{
		MGlobal::displayInfo("Error fetching vertex list!");
	}
}

//Material Micro Data
void pSendConnectedShader(MObject& object)
{
	MStatus res;
	MString debugString;
	MFnMesh mesh(object);
	std::string uuid = mesh.uuid().asString().asChar();
	size_t uuidSize = uuid.size();

	MObjectArray shaderEngineArray;
	MIntArray faceIndiciesArray;
	mesh.getConnectedShaders(0, shaderEngineArray, faceIndiciesArray);
	size_t shaderCount = shaderEngineArray.length();
	//ComPtr<char> msg {};
	//size_t messageSize = 0;

	//memcpy(msg, &uuidSize, STSIZE);
	//messageSize += STSIZE;
	//memcpy(msg + messageSize, &uuid[0], uuidSize);
	//messageSize += uuidSize;
	//memcpy(msg, &shaderCount, STSIZE);
	//messageSize += STSIZE;

	for (UINT i = 0; i < shaderCount; ++i)
	{
		MFnDependencyNode shader(shaderEngineArray[i]);
		std::string shaderUuid = shader.uuid().asString().asChar();
		size_t shaderUuidSize = shaderUuid.size();
		//memcpy(msg + messageSize, &shaderUuidSize, STSIZE);
		//messageSize += STSIZE;
		//memcpy(msg + messageSize, &shaderUuid[0], shaderUuidSize);
		//messageSize += shaderUuidSize;
	}
	//comlib.send(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::MESHSHADERS, messageSize);
}

//Type Data
void pAllocNode(MObject& object)
{
	MFnDependencyNode node { object };
	MString nodeUuid	{ node.uuid() };
	MString nodeName	{ node.name() };
	MString nodeType	{ node.typeName() };

	if (nodeName.length() == 0)
	{
		MGlobal::displayInfo("DAG");
		MFnDagNode dag{ object };
		MString dagUuid	{ dag.uuid()};
		MString dagName	{ dag.name() };
		MString dagType	{ dag.typeName() };

		std::string name	{ dagName.asChar() };
		size_t nameSize		{ name.size() };
		std::string uuid	{ dagUuid.asChar() };
		size_t uuidSize		{ uuid.size() };
		std::string type	{ dagType.asChar() };
		size_t typeSize		{ type.size() };
		
		std::vector<char> msg {};
		size_t messageSize {};

		msg.resize(
			msg.capacity() + 
			(STSIZE * 3) + 
			typeSize + 
			nameSize + 
			uuidSize
		);
		memcpy(msg.data(), &typeSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &type[0], typeSize);
		messageSize += typeSize;
		
		memcpy(msg.data() + messageSize, &nameSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &name[0], nameSize);
		messageSize += nameSize;
		
		memcpy(msg.data() + messageSize, &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;

		comlib.send(
			msg.data(),
			ComLib::MSG_TYPE::ALLOCATE,
			ComLib::ATTRIBUTE_TYPE::NODE,
			messageSize);
	}
	else
	{
		std::string name	{ nodeName.asChar() };
		size_t nameSize		{ name.size() };
		std::string uuid	{ nodeUuid.asChar() };
		size_t uuidSize		{ uuid.size() };
		std::string type	{ nodeType.asChar() };
		size_t typeSize		{ type.size() };

		std::vector<char> msg {}; 
		size_t messageSize {};

		msg.resize(
			msg.capacity() +
			(STSIZE * 3) +
			typeSize +
			nameSize +
			uuidSize
		);


		memcpy(msg.data(), &typeSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &type[0], typeSize);
		messageSize += typeSize;
		
		memcpy(msg.data() + messageSize, &nameSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &name[0], nameSize);
		messageSize += nameSize;
		
		memcpy(msg.data() + messageSize, &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;
		MGlobal::displayInfo("#####");
		comlib.send(
			msg.data(),
			ComLib::MSG_TYPE::ALLOCATE,
			ComLib::ATTRIBUTE_TYPE::NODE,
			messageSize);
	}
}
void pSendParentsData(MObject& object)
{
	MFnDagNode dag(object);
	std::string targetUuid = dag.uuid().asString().asChar();
	size_t targetUuidSize = targetUuid.size();

	std::unique_ptr<char> msg {};
	size_t messageSize = 0;

	memcpy(msg.get(), &targetUuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.get() + messageSize, &targetUuid[0], targetUuidSize);
	messageSize += targetUuidSize;

	size_t parentCount = dag.parentCount();
	memcpy(msg.get() + messageSize, &parentCount, STSIZE);
	messageSize += STSIZE;

	for (UINT i = 0; i < parentCount; ++i)
	{
		MFnDependencyNode parentNode(dag.parent(i));
		MString nodeUuid(parentNode.uuid());
		MString nodeName(parentNode.name());

		if (nodeName.length() == 0)
		{
			MFnDagNode parentDag(dag.parent(i));
			std::string uuid = parentDag.uuid().asString().asChar();
			size_t uuidSize = uuid.size();

			//memcpy(msg + messageSize, &uuidSize, STSIZE);
			//messageSize += STSIZE;
			//memcpy(msg + messageSize, &uuid[0], uuidSize);
			//messageSize += uuidSize;
		}
		else
		{
			std::string uuid = nodeUuid.asChar();
			size_t uuidSize = uuid.size();

			//memcpy(msg + messageSize, &uuidSize, STSIZE);
			//messageSize += STSIZE;
			//memcpy(msg + messageSize, &uuid[0], uuidSize);
			//messageSize += uuidSize;
		}
	}

	//comlib.send(&msg, ComLib::MSG_TYPE::ADDVALUES, ComLib::ATTRIBUTE_TYPE::PARENT, messageSize);
}

void pSendTransformData(MObject& object)
{
	pSendMatrixData(object);
}
void pSendMeshData(MObject& object)
{
	MStatus res;
	MString debugString;
	MFnMesh mesh(object);
	std::string uuid {mesh.uuid().asString().asChar()};
	size_t uuidSize {uuid.size()};

	// Will not be sent individually due to attribute callbacks.
	// Look for possible solution for separation in the future.
	pSendVertexData(object);
	//pSendVertexData2(object);

	MObjectArray shaderEngineArray;
	MIntArray faceIndiciesArray;
	mesh.getConnectedShaders(0, shaderEngineArray, faceIndiciesArray);
	size_t shaderCount = shaderEngineArray.length();
	
	std::vector<std::string> shaderUuids {};
	std::vector<size_t> shaderUuidSizes {};
	size_t totalShaderUuidSize {};

	for (UINT i = 0; i < shaderCount; ++i)
	{
		MFnDependencyNode shader(shaderEngineArray[i]);
		std::string shaderUuid = shader.uuid().asString().asChar();
		size_t shaderUuidSize = shaderUuid.size();

		shaderUuids.emplace_back(shaderUuid);
		shaderUuidSizes.emplace_back(shaderUuidSize);
		totalShaderUuidSize += shaderUuidSize;
	}

	std::vector<char> msg {};
	size_t messageSize {};
	msg.resize(
		(STSIZE * (shaderCount + 2)) +
		uuidSize + 
		totalShaderUuidSize
	);

	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;

	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;

	memcpy(msg.data() + messageSize, &shaderCount, STSIZE);
	messageSize += STSIZE;

	for (UINT i = 0; i < shaderCount; ++i)
	{
		memcpy(msg.data() + messageSize, &shaderUuidSizes[i], STSIZE);
		messageSize += STSIZE;

		memcpy(msg.data() + messageSize, &shaderUuids[i][0], shaderUuidSizes[i]);
		messageSize += shaderUuidSizes[i];
	}

	comlib.send(
		msg.data(),
		ComLib::MSG_TYPE::UPDATEVALUES, 
		ComLib::ATTRIBUTE_TYPE::MESHSHADERS,
		messageSize
	);
}
void pSendCameraData(MObject& object)
{
	pSendProjectionMatrix(object);
	//pSendCamData(object);
}
void pSendAmbientLightData(MObject& object)
{

}
void pSendPointLightData(MObject& object)
{
	MStatus res {};
	MString debugString {};
	MFnPointLight pointLightNode {object};
	std::string uuid {pointLightNode.uuid().asString().asChar()};
	size_t uuidSize {uuid.size()};

	debugString = pointLightNode.decayRate();
	MGlobal::displayInfo(debugString);
	std::vector<char> msg {};
	size_t messageSize {};
	
	float intensity {pointLightNode.intensity()};
	float color[3] { 
		pointLightNode.color().r, 
		pointLightNode.color().g,
		pointLightNode.color().b 
	};
	
	msg.resize(
		STSIZE +
		uuidSize +
		sizeof(float) +
		sizeof(float[3])
	);
	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;
	memcpy(msg.data() + messageSize, &intensity, sizeof(float));
	messageSize += sizeof(float);
	memcpy(msg.data() + messageSize, &color, sizeof(color));
	messageSize += sizeof(color);
	
	comlib.send(
		msg.data(),
		ComLib::MSG_TYPE::UPDATEVALUES,
		ComLib::ATTRIBUTE_TYPE::POINTINTENSITY,
		messageSize
	);

}
void pSendTextureData(MObject& object)
{
	MStatus res{};
	MString debugString{};
	std::vector<char> msg {};
	size_t messageSize{};

	MFnDependencyNode textureNode	{ object };
	std::string uuid { textureNode.uuid().asString().asChar() };
	size_t uuidSize{ uuid.size() };
	bool existingTexture{};

	MPlug fileNamePlug { textureNode.findPlug("fileTextureName", res) };
	if (res == MS::kSuccess)
	{
		MString texturePathString{};
		fileNamePlug.getValue(texturePathString);
		
		if (texturePathString.length() > 0)
		{		
			std::string texturePath{ texturePathString.asChar() };
			size_t pathSize{ texturePath.size() };
			existingTexture = 1;

			msg.resize(
				(STSIZE * 2) +
				uuidSize +
				sizeof(bool) +
				pathSize
			);
			memcpy(msg.data(), &uuidSize, STSIZE);
			messageSize += STSIZE;
			memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
			messageSize += uuidSize;
			
			memcpy(msg.data() + messageSize, &existingTexture, sizeof(bool));
			messageSize += sizeof(bool);

			memcpy(msg.data() + messageSize, &pathSize, STSIZE);
			messageSize += STSIZE;
			memcpy(msg.data() + messageSize, &texturePath[0], pathSize);
			messageSize += pathSize;

			comlib.send(
				msg.data(), 
				ComLib::MSG_TYPE::UPDATEVALUES, 
				ComLib::ATTRIBUTE_TYPE::TEXPATH, 
				messageSize);
		}
	}
}
void pSendBumpData(MObject& object)
{
	MStatus res{};
	MFnDependencyNode bumpMap{object};
	std::string bumpUuid{ bumpMap.uuid().asString().asChar() };
	size_t bumpUuidSize  { bumpUuid.size() };
	size_t messageSize	 {};

	MPlug bumpValuePlug	{ bumpMap.findPlug("bumpValue", res) };
	if (res == MS::kSuccess)
	{
		MPlugArray textureFileArray{};
		bumpValuePlug.connectedTo(textureFileArray, true, false);
		size_t filesLength{ textureFileArray.length() };

		std::vector<std::string> textureUuids {};
		std::vector<size_t> textureUuidSizes {};
		size_t totalTextureUuidSize {};

		for (UINT i = 0; i < filesLength; ++i)
		{
			MFnDependencyNode textureNode{ textureFileArray[i].node() };
			std::string textureUuid{ textureNode.uuid().asString().asChar() };
			size_t textureUuidSize{ textureUuid.size() };

			textureUuids.emplace_back(textureUuid);
			textureUuidSizes.emplace_back(textureUuidSize);
			totalTextureUuidSize += textureUuidSize;
		}
		std::vector<char> msg {};
		msg.resize(
			(STSIZE * (2 + filesLength)) + 
			bumpUuidSize +
			totalTextureUuidSize
		);
		memcpy(msg.data(), &bumpUuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &bumpUuid[0], bumpUuidSize);
		messageSize += bumpUuidSize;

		memcpy(msg.data() + messageSize, &filesLength, STSIZE);
		messageSize += STSIZE;

		for (UINT i = 0; i < filesLength; ++i)
		{
			memcpy(msg.data() + messageSize, &textureUuidSizes[i], STSIZE);
			messageSize += STSIZE;
			memcpy(msg.data() + messageSize, &textureUuids[i][0], textureUuidSizes[i]);
			messageSize += textureUuidSizes[i];
		}

		comlib.send(
			msg.data(), 
			ComLib::MSG_TYPE::UPDATEVALUES,
			ComLib::ATTRIBUTE_TYPE::BUMPTEXTURE,
			messageSize);
	}
}
void pSendMaterialData(MObject& object)
{
	MStatus res{};
	MString debugString{};

	MFnDependencyNode materialNode{ object };
	MString materialUuid{ materialNode.uuid() };
	std::string matUuid{ materialUuid.asChar() };
	size_t matUuidSize{ matUuid.size() };

	// Materials can be accessed when no info is created. If for safety
	if (materialNode.name().length() != 0)
	{
		MPlug colorPlug{ materialNode.findPlug("color", res) };
		if (colorPlug.asMObject().apiType() != MFn::kInvalid)
		{
			pSendPlugData(colorPlug, materialUuid, ComLib::ATTRIBUTE_TYPE::SHCOLOR);

			if (colorPlug.isConnected())
			{
				bool connection{ 1 };

				MPlugArray textures{};
				colorPlug.connectedTo(textures, true, false);
				size_t textureLength{ textures.length() };
				std::vector<std::string> textureUuids{};
				std::vector<size_t> textureUuidSizes{};
				size_t totalTextureUuidSize {};

				for (UINT i = 0; i < textureLength; ++i)
				{
					MFnDependencyNode texture{ textures[i].node() };
					std::string textureUuid{ texture.uuid().asString().asChar() };
					size_t textureUuidSize{ textureUuid.size() };
					
					textureUuids.emplace_back(textureUuid);
					textureUuidSizes.emplace_back(textureUuidSize);
					totalTextureUuidSize += textureUuidSize;
				}

				std::vector<char> msg {};
				size_t messageSize {};
				
				msg.resize(
					STSIZE * (2 + textureLength) + 
					matUuidSize + 
					sizeof(bool) +
					totalTextureUuidSize
				);
				memcpy(msg.data(), &matUuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &matUuid[0], matUuidSize);
				messageSize += matUuidSize;
				memcpy(msg.data() + messageSize, &connection, sizeof(bool));
				messageSize += sizeof(bool);
				memcpy(msg.data() + messageSize, &textureLength, STSIZE);
				messageSize += STSIZE;

				for (UINT i = 0; i < textureLength; ++i)
				{
					memcpy(msg.data() + messageSize, &textureUuidSizes[i], STSIZE);
					messageSize += STSIZE;
					memcpy(msg.data() + messageSize, &textureUuids[i][0], textureUuidSizes[i]);
					messageSize += textureUuidSizes[i];
				}

				comlib.send(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::SHCOLORMAP,
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s color is not connected!";
				MGlobal::displayInfo(debugString);
			}
		}

		MPlug ambientColorPlug = materialNode.findPlug("ambientColor", res);
		if (ambientColorPlug.asMObject().apiType() != MFn::kInvalid)
		{
			pSendPlugData(ambientColorPlug, materialUuid, ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLOR);

			if (ambientColorPlug.isConnected())
			{
				bool connection{ 1 };
				
				MPlugArray textures{};
				ambientColorPlug.connectedTo(textures, true, false);
				size_t textureLength{ textures.length() };
				std::vector<std::string> textureUuids{};
				std::vector<size_t> textureUuidSizes{};
				size_t totalTextureUuidSize{};

				for (UINT i = 0; i < textureLength; ++i)
				{
					MFnDependencyNode texture{ textures[i].node() };
					std::string textureUuid{ texture.uuid().asString().asChar() };
					size_t texUuidSize {textureUuid.size()};

					textureUuids.emplace_back(textureUuid);
					textureUuidSizes.emplace_back(texUuidSize);
					totalTextureUuidSize += texUuidSize;
				}

				std::vector<char> msg {};
				size_t messageSize{};

				msg.resize( 
					STSIZE *(2 + textureLength) + 
					matUuidSize + 
					sizeof(bool) + 
					totalTextureUuidSize
				);

				memcpy(msg.data(), &matUuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &matUuid[0], matUuidSize);
				messageSize += matUuidSize;
				memcpy(msg.data() + messageSize, &connection, sizeof(bool));
				messageSize += sizeof(bool);

				memcpy(msg.data() + messageSize, &textureLength, STSIZE);
				messageSize += STSIZE;

				for (UINT i = 0; i < textureLength; ++i)
				{
					memcpy(msg.data() + messageSize, &textureUuidSizes[i], STSIZE);
					messageSize += STSIZE;
					memcpy(msg.data() + messageSize, &textureUuids[i][0], textureUuidSizes[i]);
					messageSize += textureUuidSizes[i];
				}

				comlib.send(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP, 
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s ambientColor is not connected!";
				MGlobal::displayInfo(debugString);
			}
		}

		MPlug transparencyPlug = materialNode.findPlug("transparency", res);
		if (transparencyPlug.asMObject().apiType() != MFn::kInvalid)
		{
			pSendPlugData(transparencyPlug, materialUuid, ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCY);

			if (transparencyPlug.isConnected())
			{
				bool connection{ 1 };
				
				MPlugArray textures{};
				transparencyPlug.connectedTo(textures, true, false);
				size_t textureLength{ textures.length() };
				std::vector<std::string> textureUuids{};
				std::vector<size_t> textureUuidSizes{};
				size_t totalTextureUuidSize{};

				for (UINT i = 0; i < textureLength; ++i)
				{
					MFnDependencyNode texture{ textures[i].node() };
					std::string textureUuid{ texture.uuid().asString().asChar() };
					size_t texUuidSize {textureUuid.size()};

					textureUuids.emplace_back(textureUuid);
					textureUuidSizes.emplace_back(texUuidSize);
					totalTextureUuidSize += texUuidSize;
				}

				std::vector<char> msg {};
				size_t messageSize{};

				msg.resize(
					STSIZE * (2 + textureLength) +
					matUuidSize +
					sizeof(bool) +
					totalTextureUuidSize
				);
				memcpy(msg.data(), &matUuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &matUuid[0], matUuidSize);
				messageSize += matUuidSize;
				memcpy(msg.data() + messageSize, &connection, sizeof(bool));
				messageSize += sizeof(bool);
				
				memcpy(msg.data() + messageSize, &textureLength, STSIZE);
				messageSize += STSIZE;

				for (UINT i = 0; i < textureLength; ++i)
				{
					memcpy(msg.data() + messageSize, &textureUuidSizes[i], STSIZE);
					messageSize += STSIZE;
					memcpy(msg.data() + messageSize, &textureUuids[i][0], textureUuidSizes[i]);
					messageSize += textureUuidSizes[i];
				}

				comlib.send(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP,
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s transparency is not connected!";
				MGlobal::displayInfo(debugString);
			}
		}

		MPlug normalMapPlug = materialNode.findPlug("normalCamera", res);
		if (normalMapPlug.asMObject().apiType() != MFn::kInvalid)
		{
			if (normalMapPlug.isConnected())
			{
				bool connection{ 1 };
				MPlugArray bumps{};
				normalMapPlug.connectedTo(bumps, true, false);
				size_t bumpsLength{ bumps.length() };
				std::vector<std::string> bumpUuids {};
				std::vector<size_t>	bumpUuidSizes {};
				size_t totalBumpUuidSizes {};

				for (UINT i = 0; i < bumpsLength; ++i)
				{
					MObject bumpObject{ bumps[i].node() };
					MFnDependencyNode bumpMap{ bumpObject };
					std::string bumpUuid{ bumpMap.uuid().asString().asChar() };
					size_t bumpUuidSize{ bumpUuid.size() };

					bumpUuids.emplace_back(bumpUuid);
					bumpUuidSizes.emplace_back(bumpUuidSize);
					totalBumpUuidSizes += bumpUuidSize;

					// Bump Node
					//Update the data for bump nodes in msg2! Since data isn't accessable earlier.
					std::vector<char> msg2 {};
					size_t messageSize2{};

					msg2.resize(
						(STSIZE * 2) + 
						bumpUuidSize + 
						matUuidSize);
					memcpy(msg2.data(), &bumpUuidSize, STSIZE);
					messageSize2 += STSIZE;
					memcpy(msg2.data() + messageSize2, &bumpUuid[0], bumpUuidSize);
					messageSize2 += bumpUuidSize;
					// Don't forget to append on receiving end if not already existing!
					memcpy(msg2.data() + messageSize2, &matUuidSize, STSIZE);
					messageSize2 += STSIZE;
					memcpy(msg2.data() + messageSize2, &matUuid[0], matUuidSize);
					messageSize2 += matUuidSize;
					comlib.send(
						msg2.data(),
						ComLib::MSG_TYPE::UPDATEVALUES,
						ComLib::ATTRIBUTE_TYPE::BUMPSHADER,
						messageSize2);
				}

				std::vector<char> msg {};
				size_t messageSize{};

				msg.resize(
					STSIZE * (2 + bumpsLength) + 
					matUuidSize + 
					sizeof(bool) +
					totalBumpUuidSizes
				);
				memcpy(msg.data(), &matUuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &matUuid[0], matUuidSize);
				messageSize += matUuidSize;
				memcpy(msg.data() + messageSize, &connection, sizeof(bool));
				messageSize += sizeof(bool);

				memcpy(msg.data() + messageSize, &bumpsLength, STSIZE);
				messageSize += STSIZE;

				for (UINT i = 0; i < bumpsLength; ++i)
				{
					memcpy(msg.data() + messageSize, &bumpUuidSizes[i], STSIZE);
					messageSize += STSIZE;
					memcpy(msg.data() + messageSize, &bumpUuids[i][0], bumpUuidSizes[i]);
					messageSize += bumpUuidSizes[i];
				}

				comlib.send(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::NORMALMAP, 
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s normalCamera is not connected!";
				MGlobal::displayInfo(debugString);
			}
		}
	}
}
void pSendShaderGroupData(MObject& object)
{
	MStatus res{};
	MString debugString {};
	MFnDependencyNode shaderGroup{ object };
	std::string uuid{ shaderGroup.uuid().asString().asChar() };
	size_t uuidSize{ uuid.size()};

	// find the connected shader
	MPlug surfaceShader{ shaderGroup.findPlug("surfaceShader", res) };
	if (surfaceShader.isConnected())
	{
		MPlugArray shaders {};
		surfaceShader.connectedTo(shaders, true, false);
		size_t shaderCount {shaders.length()};
		std::vector<std::string> shaderUuids {};
		std::vector<size_t> shaderUuidSizes {};
		size_t totalShaderUuidSize {};

		for (UINT i = 0; i < shaderCount; ++i)
		{
			MFnDependencyNode shader {shaders[i].node()};
			std::string shaderUuid {shader.uuid().asString().asChar()};
			size_t shaderUuidSize {shaderUuid.size()};
			
			shaderUuids.emplace_back(shaderUuid);
			shaderUuidSizes.emplace_back(shaderUuidSize);
			totalShaderUuidSize += shaderUuidSize;
		}

		std::vector<char> msg {};
		size_t messageSize {};

		msg.resize(
			(sizeof(size_t)) * (2 + shaderCount) +
			uuidSize +
			totalShaderUuidSize
		);

		memcpy(msg.data(), &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;
		memcpy(msg.data() + messageSize, &shaderCount, STSIZE);
		messageSize += STSIZE;
		
		for (UINT i = 0; i < shaderCount; ++i)
		{
			memcpy(msg.data() + messageSize, &shaderUuidSizes[i], STSIZE);
			messageSize += STSIZE;

			memcpy(msg.data() + messageSize, &shaderUuids[i][0], shaderUuidSizes[i]);
			messageSize += shaderUuidSizes[i];
		}

		comlib.send(
			msg.data(),
			ComLib::MSG_TYPE::UPDATEVALUES,
			ComLib::ATTRIBUTE_TYPE::SESURFACE,
			messageSize);

		// No Access to connected meshes!
		// Connections have to be collected and appended through connectedShaders in meshes!
		// Figure out a fix, no point having shader engine otherwise.
		/*
			MFnSet meshSet(shaderGroup.findPlug("dagSetMembers", res));
			debugString = meshSet.attributeCount();
			MGlobal::displayInfo(debugString);
			MPlugArray meshes;
			meshSet.getConnections(meshes);
			debugString = meshes.length();
			MGlobal::displayInfo(debugString);

			MPlug meshPlugList(shaderGroup.findPlug("dagSetMembers", res));
			debugString = meshPlugList.numChildren();
			MGlobal::displayInfo(debugString);
			for (size_t i = 0; i < meshPlugList.numChildren(); ++i)
			{
				MPlug meshPlug(meshPlugList.child(i));
				debugString = meshPlug.name();
				MGlobal::displayInfo(debugString);
			}
		*/
	}
	else
	{
		debugString = "No Material connected to SE: ";
		debugString += uuid.c_str();
		MGlobal::displayInfo(debugString);
	}

}

void pNameChangeCallback(MObject& object, const MString& lastName, void* clientData)
{
	MStatus res;
	MString debugString;
	MFnDependencyNode node(object);
	std::string name = node.name().asChar();
	size_t nameSize = name.size();
	std::string nodeLastName = lastName.asChar();
	size_t lastNameSize = nodeLastName.size();
	//ComPtr<char> msg {};
	//size_t messageSize = 0;
	//
	//memcpy(msg, &lastNameSize, STSIZE);
	//messageSize += STSIZE;
	//memcpy(msg + messageSize, &nodeLastName[0], lastNameSize);
	//messageSize += lastNameSize;
	//memcpy(msg + messageSize, &nameSize, STSIZE);
	//messageSize += STSIZE;
	//memcpy(msg + messageSize, &name[0], nameSize);
	//messageSize += nameSize;
	//
	//comlib.send(msg, ComLib::MSG_TYPE::RENAME, ComLib::ATTRIBUTE_TYPE::NODENAME, messageSize);
}
void pUuidChangeCallback(MObject& object, const MUuid& lastUuid, void* clientData)
{
	MStatus res;
	MString debugString;
	MFnDependencyNode node(object);
	std::string uuid = node.uuid().asString().asChar();
	size_t uuidSize = uuid.size();
	std::string nodeLastUuid = lastUuid.asString().asChar();
	size_t lastUuidSize = nodeLastUuid.size();
	//ComPtr<char> msg {};
	//size_t messageSize = 0;
	//
	//memcpy(msg, &lastUuidSize, STSIZE);
	//messageSize += STSIZE;
	//memcpy(msg + messageSize, &nodeLastUuid[0], lastUuidSize);
	//messageSize += lastUuidSize;
	//memcpy(msg + messageSize, &uuidSize, STSIZE);
	//messageSize += STSIZE;
	//memcpy(msg + messageSize, &uuid[0], uuidSize);
	//messageSize += uuidSize;
	//
	//comlib.send(msg, ComLib::MSG_TYPE::RENAME, ComLib::ATTRIBUTE_TYPE::NODEUUID, messageSize);
}

void pAttributeAddedRemovedCallback(MNodeMessage::AttributeMessage msg, MPlug& plug, void* clientData)
{
	MStatus res;
	MString debugString;

	MFnDependencyNode node(plug.node());
	MObject nodeObject(node.object());
	MFnDagNode dag(node.object());
	MFn::Type ownerType(plug.node().apiType());

	MFnAttribute attribute(plug);
	MObject attributeObject(attribute.object());
	MString attributeName(attribute.name().asChar());

	debugString = "OWNER NAME: ";
	debugString += node.name();
	MGlobal::displayInfo(debugString);
	
	debugString = "ATTRIBUTE NAME: ";
	debugString += attributeName;
	MGlobal::displayInfo(debugString);

	MGlobal::displayInfo("##########  ATTRIBUTE ADDED #########");
}
void pAttributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	MStatus res;
	MString debugString;

	MFnDependencyNode node(plug.node());
	MObject nodeObject(node.object());
	MFnDagNode dag(node.object());
	MFn::Type ownerType(plug.node().apiType());

	MFnAttribute attribute(plug);
	MObject attributeObject(attribute.object());
	MString attributeName(attribute.name().asChar());

	debugString = "OWNER NAME: ";
	debugString += node.name();
	MGlobal::displayInfo(debugString);
	
	debugString = "ATTRIBUTE NAME: ";
	debugString += attributeName;
	MGlobal::displayInfo(debugString);

	if (msg & MNodeMessage::AttributeMessage::kAttributeEval ||
		msg & MNodeMessage::AttributeMessage::kAttributeSet)
	{
		switch (ownerType)
		{
		case MFn::kTransform:
			if (attributeName == "translate" ||
				attributeName == "translateX" ||
				attributeName == "translateY" ||
				attributeName == "translateZ" ||
				attributeName == "translateX" ||
				attributeName == "rotate" ||
				attributeName == "rotateX" ||
				attributeName == "rotateY" ||
				attributeName == "rotateZ" ||
				attributeName == "scale" ||
				attributeName == "scaleX" ||
				attributeName == "scaleY" ||
				attributeName == "scaleZ")
			{
				pSendMatrixData(nodeObject);
			}
			break;
		case MFn::kCamera:
			if (attributeName == "orthographicWidth" ||
				attributeName == "orthographic" ||
				attributeName == "focalLength" ||
				attributeName == "farClipPlane" ||
				attributeName == "nearClipPlane" ||
				attributeName == "cameraScale" ||
				attributeName == "horizontalFilmAperture" ||
				attributeName == "verticalFilmAperture" ||
				attributeName == "lensSqueezeRatio" ||
				attributeName == "horizontalShake" ||
				attributeName == "verticalShake" ||
				attributeName == "filmTranslateH" ||
				attributeName == "filmTranslateV" ||
				attributeName == "postScale")
			{
				pSendProjectionMatrix(nodeObject);
			}
			break;
		case MFn::kMesh:
			if (/*attributeName == "pnts" ||
				attributeName == "uvPivot" ||
				attributeName == "vertexNormal"*/
				attributeName == "outMesh")
			{
				//pSendVertexData(nodeObject);
			}
			break;
		case MFn::kPointLight:
			if (attributeName == "intensity" ||
				attributeName == "colorR" ||
				attributeName == "colorG" || 
				attributeName == "colorB" ||
				attributeName == "decayRate")
			{
				pSendPointLightData(nodeObject);
			}
			break;
		case MFn::kLambert:
		case MFn::kBlinn:
			if (attributeName == "color" ||
				attributeName == "transparency" ||
				attributeName == "ambientColor" ||
				attributeName == "normalCamera")
			{
				pSendMaterialData(nodeObject);
			}
			break;
		case MFn::kFileTexture:
			if (attributeName == "fileTextureName")
			{
				pSendTextureData(nodeObject);
			}
			break;
		default:
			break;
		}
	}
	else if (msg & MNodeMessage::AttributeMessage::kConnectionMade)
	{
		debugString = "Connected ATTRIBUTE NAME: ";
		debugString += attributeName;
		MGlobal::displayInfo(debugString);

		switch (ownerType)
		{
		case MFn::kMesh:
			if (attributeName == "instObjGroups")
			{
				pSendConnectedShader(nodeObject);
			}
			break;
		case MFn::kBump:
			if (attributeName == "bumpValue")
			{
				pSendBumpData(nodeObject);
			}
			break;
		case MFn::kLambert:
		case MFn::kBlinn:
			if (attributeName == "color" ||
				attributeName == "transparency" || 
				attributeName == "ambientColor" ||
				attributeName == "normalCamera")
			{
				pSendMaterialData(nodeObject);
			}
			break;
		case MFn::kShadingEngine:
			if (attributeName == "surfaceShader")
			{
				pSendShaderGroupData(nodeObject);
			}
			break;
		default:
			break;
		}
		
	}
	else if (msg & MNodeMessage::AttributeMessage::kConnectionBroken)
	{

	}
}

void pParentAddedCallback(MDagPath& child, MDagPath& parent, void* clientData)
{
	MFnDagNode targetDag(child);
	std::string targetUuid(targetDag.uuid().asString().asChar());
	size_t targetUuidSize = targetUuid.size();
	
	MFnDagNode parentDag(parent);
	std::string parentUuid(parentDag.uuid().asString().asChar());
	size_t parentUuidSize = parentUuid.size();

	if (targetDag.name().length() != 0 || parentDag.name().length() != 0)
	{
		//ComPtr<char> msg {};
		//size_t messageSize = 0;
		//
		//memcpy(msg, &targetUuidSize, STSIZE);
		//messageSize += STSIZE; 
		//memcpy(msg + messageSize, &targetUuid[0], targetUuidSize);
		//messageSize += targetUuidSize;
		//
		//memcpy(msg + messageSize, &parentUuidSize, STSIZE);
		//messageSize += STSIZE;
		//memcpy(msg + messageSize, &parentUuid[0], parentUuidSize);
		//messageSize += parentUuidSize;
		//
		//comlib.send(&msg, ComLib::MSG_TYPE::ADDVALUES, ComLib::ATTRIBUTE_TYPE::PARENT, messageSize);
	}
}
void pParentRemovedCallback(MDagPath& child, MDagPath& parent, void* clientData)
{
	MFnDagNode targetDag(child);
	std::string targetUuid(targetDag.uuid().asString().asChar());
	size_t targetUuidSize = targetUuid.size();

	MFnDagNode parentDag(parent);
	std::string parentUuid(parentDag.uuid().asString().asChar());
	size_t parentUuidSize = parentUuid.size();
	
	if (targetDag.name().length() != 0 || parentDag.name().length() != 0)
	{
		//ComPtr<char> msg {};
		//size_t messageSize = 0;
		//
		//memcpy(msg, &targetUuidSize, STSIZE);
		//messageSize += STSIZE; 
		//memcpy(msg + messageSize, &targetUuid[0], targetUuidSize);
		//messageSize += targetUuidSize;
		//
		//memcpy(msg + messageSize, &parentUuidSize, STSIZE);
		//messageSize += STSIZE;
		//memcpy(msg + messageSize, &parentUuid[0], parentUuidSize);
		//messageSize += parentUuidSize;
		//
		//comlib.send(&msg, ComLib::MSG_TYPE::REMOVEVALUES, ComLib::ATTRIBUTE_TYPE::PARENT, messageSize);
	}
}

void pAddNodeCallbacks(MObject& object)
{
	MStatus res;
	MCallbackId NameChangedID = MNodeMessage::addNameChangedCallback(
		object,
		pNameChangeCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(NameChangedID) == MS::kSuccess) {};
	}

	MCallbackId UuidChangedID = MNodeMessage::addUuidChangedCallback(
		object,
		pUuidChangeCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(UuidChangedID) == MS::kSuccess) {};
	}

	MCallbackId AttributeChangedID = MNodeMessage::addAttributeChangedCallback(
		object,
		pAttributeChangedCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(AttributeChangedID) == MS::kSuccess) {};
	}
	MCallbackId AttributeAddedRemovedID = MNodeMessage::addAttributeAddedOrRemovedCallback(
	object,
	pAttributeAddedRemovedCallback,
	NULL,
	&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(AttributeAddedRemovedID) == MS::kSuccess) {};
	}
}

void pNodeCreationCallback(MObject& object, void* clientData)
{
	MStatus res;
	MString debugString;
	MFnDependencyNode node(object);

	MGlobal::displayInfo(node.typeName());

	//switch (object.apiType())
	//{
	//case MFn::kTransform:
	//	pAddNodeCallbacks(object);
	//	pAllocNode(object);
	//	pSendTransformData(object);
	//case MFn::kMesh:
	//	pAddNodeCallbacks(object);
	//	pAllocNode(object);
	//	break;
	//case MFn::kCamera:
	//	pAddNodeCallbacks(object);
	//	pAllocNode(object);
	//	pSendCameraData(object);
	//	break;
	//case MFn::kLight:
	//	pAddNodeCallbacks(object);
	//	pAllocNode(object);
	//	break;
	//case MFn::kShadingEngine:
	//	pAddNodeCallbacks(object);
	//	pAllocNode(object);
	//	pSendShaderGroupData(object);
	//	break;

	//	// Surface Materials
	//case MFn::kLambert:
	//case MFn::kBlinn:
	//case MFn::kPhong:
	//case MFn::kPhongExplorer:
	//case MFn::kLayeredShader:
	//case MFn::kRampShader:
	//case MFn::kShadingMap:
	//case MFn::kHairTubeShader:
	//	pAddNodeCallbacks(object);
	//	pAllocNode(object);
	//	pSendMaterialData(object);
	//	break;
	//	////Odd Materials
	//	//case MFn::kSurfaceShader:
	//	//case MFn::kVolumeShader:
	//	//	break;
	//	//// Custom Materials (custom, StingrayPBR, DirectX
	//	//case MFn::kPluginHardwareShader:
	//	//case MFn::kPluginDependNode:
	//	//	//Not yet supported
	//	//	break;
	//	//
	//	//// Utils Material
	//	//case MFn::kUseBackground:
	//	//case MFn::kAnisotropy:
	//	//case MFn::kEnvFogMaterial:
	//	//case MFn::kFluid:
	//	//case MFn::kLightFogMaterial:
	//	//case MFn::kOceanShader:
	//	//case MFn::kDisplacementShader:
	//	//case MFn::kBulge:
	//	//case MFn::kChecker:
	//	//case MFn::kCloth:
	//	//case MFn::kFluidTexture2D:

	//	//	break;
	//case MFn::kFileTexture:
	//	pAddNodeCallbacks(object);
	//	pAllocNode(object);
	//	pSendTextureData(object);
	//	break;
	//default:
	//	break;
	//}
}
void pNodeDeleteCallback(MObject& object, void* clientData)
{
	MStatus res;
	MString debugString;
	MFnDependencyNode node(object);
	std::string uuid = node.uuid().asString().asChar();
	size_t uuidSize = uuid.size();
	//ComPtr<char> msg {};
	//size_t messageSize = 0;
	//
	//memcpy(msg, &uuidSize, STSIZE);
	//messageSize += STSIZE;
	//memcpy(msg + messageSize, &uuid[0], uuidSize);
	//messageSize += uuidSize;
	//
	//comlib.send(msg, ComLib::MSG_TYPE::DEALLOCATE, ComLib::ATTRIBUTE_TYPE::NODE, messageSize);
}

void pGetChildParenting(MObject& currentObject)
{
	MString debugString;
	MStatus res;

	MFnDagNode currentDag(currentObject);

	for (UINT i = 0; i < currentDag.childCount(); ++i)
	{
		MObject childObject(currentDag.child(i));
		MFnDagNode childDAG(childObject);

		switch (childObject.apiType())
		{
		case MFn::kTransform:
		case MFn::kMesh:
		case MFn::kCamera:
		case MFn::kPointLight:
			pSendParentsData(childObject);
			break;
		default:
			break;
		}

		if (childDAG.childCount() > 0)
		{
			pGetChildParenting(childObject);
		}
	}

}
void pGetMaterialParenting()
{
	MStatus res;
	MString debugString;

	//get Materials | HARDCODED (fix?)
	std::vector<MFn::Type> type =
	{
		MFn::kLambert,
		MFn::kBlinn,
		MFn::kPhong,
		MFn::kPhongExplorer,
		MFn::kLayeredShader,
		//MFn::kSurfaceShader,
		MFn::kRampShader,
		MFn::kShadingMap,
		//MFn::kVolumeShader,
		MFn::kHairTubeShader,
		//MFn::kPluginHardwareShader,
		//MFn::kPluginDependNode
	};

	MItDependencyNodes fileIt(MFn::kFileTexture);
	for (; !fileIt.isDone(); fileIt.next())
	{
		MObject textureObject(fileIt.thisNode());
		pSendParentsData(textureObject);
	}

	//Bumps fetched through shader enigine

	// Iterate through shaders
	for (size_t i = 0; i < type.size(); ++i)
	{
		MItDependencyNodes shaderIt(type.at(i));
		for (; !shaderIt.isDone(); shaderIt.next())
		{
			MObject shaderObject(shaderIt.thisNode());
			pSendParentsData(shaderObject);
		}
	}

	// Iterate through ShaderEngines(SE) (For sorting meshes and shaders, several meshes can share one shader)
	MItDependencyNodes engineIt(MFn::kShadingEngine);
	for (; !engineIt.isDone(); engineIt.next())
	{
		MObject groupObject(engineIt.thisNode());
		MFnDependencyNode engine(groupObject);
		// Make sure not to include SE for particles or other unecessary types (unless wanted). Filter here.
		if (engine.name() != "initialParticleSE")
		{
			pAddNodeCallbacks(groupObject);
			pAllocNode(groupObject);

			pSendShaderGroupData(groupObject);
		}
	}

}
void pGetParentChildRelationship()
{
	MString debugString;
	MStatus res;

	//ComPtr<char> msg {};
	size_t messageSize = 0;

	//comlib.send(msg.Get(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::EXSTART, messageSize);

	//Queries the current viewport.
	M3dView sceneView;
	sceneView = sceneView.active3dView();

	//Queries the active cameras DagPath.
	MDagPath camShapeDagPath;
	sceneView.getCamera(camShapeDagPath);

	//Get root node
	MFnDagNode camDAG(camShapeDagPath.node());
	MFnDagNode rootDAG(camDAG.dagRoot());

	pGetMaterialParenting();

	for (UINT i = 0; i < rootDAG.childCount(); ++i)
	{
		MFnDagNode rootChildDAG(rootDAG.child(i));
		if (rootChildDAG.typeName() == "transform")
		{
			MObject rootChildObject(rootChildDAG.object());
			MString rootChildName(rootChildDAG.name());

			//Hard-coded. Possible automation
			//Filter internal essencial scene objects, carefull when modifying
			if (rootChildName != "groundPlane_transform"
				&& rootChildName != "defaultUfeProxyParent"
				&& rootChildName != "shaderBallCamera1"
				&& rootChildName != "shaderBallOrthoCamera1"
				&& rootChildName != "MayaMtlView_FillLight1"
				&& rootChildName != "MayaMtlView_RimLight1")
			{
				pSendParentsData(rootChildObject);

				if (rootChildDAG.childCount())
				{
					pGetChildParenting(rootChildObject);
				}
			}
		}
	}
}

void pCreateAddCallbackChildNode(MObject& currentObject)
{
	MString debugString;
	MStatus res;

	MFnDagNode currentDag(currentObject);

	for (UINT i = 0; i < currentDag.childCount(); ++i)
	{
		MObject childObject(currentDag.child(i));
		MFnDagNode childDAG(childObject);

		switch (childObject.apiType())
		{
		case MFn::kTransform:
			//pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			//pSendTransformData(childObject);
			break;

		case MFn::kMesh:
			//pAddNodeCallbacks(childObject);
			//pAllocNode(childObject);
			//pSendMeshData(childObject);
			break;

		case MFn::kCamera:
			//pAddNodeCallbacks(childObject);
			//pAllocNode(childObject);
			//pSendCameraData(childObject);
			break;

		case MFn::kPointLight:
			//pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendPointLightData(childObject);
			break;
		default:
			break;
		}

		if (childDAG.childCount() > 0)
		{
			pCreateAddCallbackChildNode(childObject);
		}
	}

}
void pGetExistingMaterials()
{
	// Due to Maya's push-pull system, the allocated nodes are ordered and collected through independent nodes first.
	MStatus res;
	MString debugString;

	//get Materials | HARDCODED (fix?)
	std::vector<MFn::Type> type = 
	{
		MFn::kLambert,
		MFn::kBlinn,
		//MFn::kPhong,
		//MFn::kPhongExplorer,
		//MFn::kLayeredShader,
		//MFn::kSurfaceShader,
		//MFn::kRampShader,
		//MFn::kShadingMap,
		//MFn::kVolumeShader,
		//MFn::kHairTubeShader,
		//MFn::kPluginHardwareShader,
		//MFn::kPluginDependNode
	};

	// Iterate though textures
	MItDependencyNodes fileIt(MFn::kFileTexture);
	for (; !fileIt.isDone(); fileIt.next())
	{
		MObject textureObject(fileIt.thisNode());
		//pAddNodeCallbacks(textureObject);
		pAllocNode(textureObject);
		pSendTextureData(textureObject);
	} 

	//Iterate through all bump nodes (Unnecessary node atm? Maya internal system, extra step.)
	MItDependencyNodes bumpIt(MFn::kBump);
	for (; !bumpIt.isDone(); bumpIt.next())
	{
		MObject bumpObject(bumpIt.thisNode());
		//pAddNodeCallbacks(bumpObject);
		pAllocNode(bumpObject);
		//
		//// Append shaders to outNormal in shaders, after pull iteration.
		// Bump output Plugs are nullptr : MS::kInvalid since pull from shader hasn't been initiated.
		pSendBumpData(bumpObject);
	}

	//Iterate through all shaders (Type list)
	for (size_t i = 0; i < type.size(); ++i)
	{
		MItDependencyNodes shaderIt(type.at(i));
		for (; !shaderIt.isDone(); shaderIt.next())
		{
			MObject shaderObject(shaderIt.thisNode());
			//pAddNodeCallbacks(shaderObject);
			pAllocNode(shaderObject);
			//
			pSendMaterialData(shaderObject);
		}
	}

	// Iterate through ShaderEngines(SE) (For sorting meshes and shaders, several meshes can share one shader)
	MItDependencyNodes engineIt(MFn::kShadingEngine);
	for (; !engineIt.isDone(); engineIt.next())
	{
		MObject groupObject(engineIt.thisNode());
		MFnDependencyNode engine(groupObject);
		// Make sure not to include SE for particles or other unecessary types (unless wanted). Filter here.
		if (engine.name() != "initialParticleSE")
		{
			//pAddNodeCallbacks(groupObject);
			pAllocNode(groupObject);
			
			pSendShaderGroupData(groupObject);
		}
	}
}
void pGetExistingScene()
{
	MString debugString;
	MStatus res;

	//Queries the current viewport.
	M3dView sceneView;
	sceneView = sceneView.active3dView();

	//Queries the active cameras DagPath.
	MDagPath camShapeDagPath;
	sceneView.getCamera(camShapeDagPath);

	//Get root node
	MFnDagNode camDAG(camShapeDagPath.node());
	MFnDagNode rootDAG(camDAG.dagRoot());

	MObject	worldMatrixObject { rootDAG.object() };
	pAllocNode(worldMatrixObject);
	pSendTransformData(worldMatrixObject);

	//pGetExistingMaterials();
	//DONE
	//Go through existing objects in the scene.
	for (UINT i = 0; i < rootDAG.childCount(); ++i)
	{
		//Will probably be a transform, if statement to ensure not to stumble upon unexpected results.
		//Since we want to find the transformer for our objects anyway. (Typename hardcoded, possible fix?)
		MFnDagNode rootChildDAG(rootDAG.child(i));
		if (rootChildDAG.typeName() == "transform")
		{
			MObject rootChildObject(rootChildDAG.object());
			MString rootChildName(rootChildDAG.name());
	
			//Hard-coded. Possible automation?
			//Filter internal essencial scene objects, carefull when modifying
			if (rootChildName != "groundPlane_transform"
				&& rootChildName != "defaultUfeProxyParent"
				&& rootChildName != "shaderBallCamera1"
				&& rootChildName != "shaderBallOrthoCamera1"
				&& rootChildName != "shaderBallGeomShape1"
				&& rootChildName != "shaderBallGeom1"
				&& rootChildName != "MayaMtlView_FillLight1"
				&& rootChildName != "MayaMtlView_RimLight1"
				&& rootChildName != "MayaMtlView_KeyLight1")
			{
				//pAddNodeCallbacks(rootChildObject);
				pAllocNode(rootChildObject);
				pSendTransformData(rootChildObject);
	
				if (rootChildDAG.childCount())
				{
					pCreateAddCallbackChildNode(rootChildObject);
				}
			}
		}
	}
	
	//pSendActiveCamera(camDAG);
}

void preRenderCallback(const MString& str, void* clientData)
{
	//Check connection status
	//In-case dissconnect or reconnect -Clear buffer
	//Fetch existant objects
}
//Read notes: change comlib.send to storeCommand(){(void*)buffer = command}
void renderChangeCallback(const MString& str, void* clientData)
{
	M3dView sceneView;
	sceneView = sceneView.active3dView();

	MDagPath camShapeDagPath;
	sceneView.getCamera(camShapeDagPath);

	MFnDagNode camDAG(camShapeDagPath.node());

	pSendActiveCamera(camDAG);

	//Send buffer comlib.send(buffer);
}

EXPORT MStatus initializePlugin(MObject obj)
{
	MStatus res{ MS::kFailure };
	MString debugString {};
	
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

	//#### Query all the existing data in the scene
	std::vector<char> msg {};
	size_t messageSize = 0;
	comlib.send(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::EXSTART, messageSize);
	
	pGetExistingScene();
	//pGetParentChildRelationship();

	comlib.send(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::EXEND, messageSize);
	//####

	//MCallbackId nodeAddedId{ MDGMessage::addNodeAddedCallback(
	//	pNodeCreationCallback,
	//	kDefaultNodeType,
	//	nullptr,
	//	&res) };
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(nodeAddedId) == MS::kSuccess) {};
	//}

	//MCallbackId nodeRemovedId{ MDGMessage::addNodeRemovedCallback(
	//	pNodeDeleteCallback,
	//	kDefaultNodeType,
	//	nullptr,
	//	&res) };
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(nodeRemovedId) == MS::kSuccess) {};
	//}

	//MCallbackId parentAddedID{ MDagMessage::addParentAddedCallback(
	//	pParentAddedCallback,
	//	nullptr,
	//	&res) };
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(parentAddedID) == MS::kSuccess) {};
	//}

	//MCallbackId parentRemovedID{ MDagMessage::addParentRemovedCallback(
	//	pParentRemovedCallback,
	//	nullptr,
	//	&res) };
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(parentAddedID) == MS::kSuccess) {};
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

	//MCallbackId preRenderId{ MUiMessage::add3dViewPreRenderMsgCallback(
	//	"modelPanel4",
	//	preRenderCallback,
	//	nullptr,
	//	&res
	//)};
	//if (res)

	//MCallbackId camTranslateId{ MUiMessage::add3dViewPostRenderMsgCallback(
	//	"modelPanel4",
	//	renderChangeCallback,
	//	nullptr,
	//	&res
	//) };

	return res;
}
EXPORT MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	MMessage::removeCallbacks(myCallbackArray);
	MGlobal::displayInfo("Maya plugin unloaded!");

	return MS::kSuccess;
}