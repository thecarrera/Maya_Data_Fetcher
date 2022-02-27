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
#include <DirectXMath.h>

//Make a vector of union, connect callbacks to nodes and uuids, [0] is initialize callbacks
MCallbackIdArray myCallbackArray;
ComLib comlib("sharedFileMap", (25ULL << 23ULL)); //200MB

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
 
void pPrintMatrix(MMatrix mat)
{
	MString debugString{};
	debugString = "\n";
	debugString += mat[0][0];
	debugString += " ";
	debugString += mat[0][1];
	debugString += " ";
	debugString += mat[0][2];
	debugString += " ";
	debugString += mat[0][3];
	debugString += "\n";

	debugString += mat[1][0];
	debugString += " ";
	debugString += mat[1][1];
	debugString += " ";
	debugString += mat[1][2];
	debugString += " ";
	debugString += mat[1][3];
	debugString += "\n";

	debugString += mat[2][0];
	debugString += " ";
	debugString += mat[2][1];
	debugString += " ";
	debugString += mat[2][2];
	debugString += " ";
	debugString += mat[2][3];
	debugString += "\n";

	debugString += mat[3][0];
	debugString += " ";
	debugString += mat[3][1];
	debugString += " ";
	debugString += mat[3][2];
	debugString += " ";
	debugString += mat[3][3];

	MGlobal::displayInfo(debugString);
}
void pPrintMatrix(double mat[4][4])
{
	MString debugString {};
	debugString = "\n";
	debugString += mat[0][0];
	debugString += " ";
	debugString += mat[0][1];
	debugString += " ";
	debugString += mat[0][2];
	debugString += " ";
	debugString += mat[0][3];
	debugString += "\n";
	
	debugString += mat[1][0];
	debugString += " ";
	debugString += mat[1][1];
	debugString += " ";
	debugString += mat[1][2];
	debugString += " ";
	debugString += mat[1][3];
	debugString += "\n";
	
	debugString += mat[2][0];
	debugString += " ";
	debugString += mat[2][1];
	debugString += " ";
	debugString += mat[2][2];
	debugString += " ";
	debugString += mat[2][3];
	debugString += "\n";
	
	debugString += mat[3][0];
	debugString += " ";
	debugString += mat[3][1];
	debugString += " ";
	debugString += mat[3][2];
	debugString += " ";
	debugString += mat[3][3];

	MGlobal::displayInfo(debugString);
}
void pPrintMatrix(DirectX::XMMATRIX mat)
{
	MString debugString{};
	debugString = "\n";
	debugString += mat.r[0].m128_f32[0];
	debugString += " ";
	debugString += mat.r[0].m128_f32[1];
	debugString += " ";
	debugString += mat.r[0].m128_f32[2];
	debugString += " ";
	debugString += mat.r[0].m128_f32[3];
	debugString += "\n";

	debugString += mat.r[1].m128_f32[0];
	debugString += " ";
	debugString += mat.r[1].m128_f32[1];
	debugString += " ";
	debugString += mat.r[1].m128_f32[2];
	debugString += " ";
	debugString += mat.r[1].m128_f32[3];
	debugString += "\n";

	debugString += mat.r[2].m128_f32[0];
	debugString += " ";
	debugString += mat.r[2].m128_f32[1];
	debugString += " ";
	debugString += mat.r[2].m128_f32[2];
	debugString += " ";
	debugString += mat.r[2].m128_f32[3];
	debugString += "\n";

	debugString += mat.r[3].m128_f32[0];
	debugString += " ";
	debugString += mat.r[3].m128_f32[1];
	debugString += " ";
	debugString += mat.r[3].m128_f32[2];
	debugString += " ";
	debugString += mat.r[3].m128_f32[3];
	debugString += "\n";

	MGlobal::displayInfo(debugString);
}

//General Micro Data
void pSendActiveCamera()
{
	MString debugString {};
	M3dView sceneView{ sceneView.active3dView() };
	MDagPath camShapeDagPath{};
	sceneView.getCamera(camShapeDagPath);
	MFnDagNode camDAG{ camShapeDagPath.node() };
	MFnCamera cam {camDAG.object()};

	std::string uuid {camDAG.uuid().asString().asChar()};
	size_t uuidSize {uuid.size()};
	std::vector<char> msg {};
	size_t messageSize {};
	//MMatrix viewMat {camShapeDagPath.inclusiveMatrix().transpose()};

	//Fetch the camera view matrix through the viewport instead of the camera.
	//Camera will give you a matrix with Translation -> Rotation order.
	//modelViewMatrix will give you Rotation -> Translation order.
	//Cons: We are forced to update the view matrix every render pass even though the camera isn't moving.
	//      NOTE: Figure out if there's a way to connect a callback for this view matrix specifically.
	//            We only want to update when the matrix has actually changed.
	MMatrix viewMat;
	sceneView.modelViewMatrix(viewMat);
	viewMat = viewMat.transpose();
	double viewMatrix[4][4] {
		viewMat.matrix[0][0],
		viewMat.matrix[0][1],
		viewMat.matrix[0][2],
		viewMat.matrix[0][3],

		viewMat.matrix[1][0],
		viewMat.matrix[1][1],
		viewMat.matrix[1][2],
		viewMat.matrix[1][3],

		viewMat.matrix[2][0] * -1.0f,
		viewMat.matrix[2][1] * -1.0f,
		viewMat.matrix[2][2] * -1.0f,
		viewMat.matrix[2][3] * -1.0f,

		viewMat.matrix[3][0],
		viewMat.matrix[3][1],
		viewMat.matrix[3][2],
		viewMat.matrix[3][3]
	};
	//pPrintMatrix(viewMatrix);

	msg.resize(
		STSIZE + 
		uuidSize + 
		sizeof(viewMatrix)
	);
	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;
	memcpy(msg.data() + messageSize, &viewMatrix, sizeof(viewMatrix));
	messageSize += sizeof(viewMatrix);
	
	comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::ACTIVECAM, ComLib::ATTRIBUTE_TYPE::NONE, messageSize);
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
		//float container[3] { fv.x, fv.y, fv.z };

		debugString = "R: ";
		debugString += fv.x;
		debugString += " G: ";
		debugString += fv.y;
		debugString += " B: ";
		debugString += fv.z;
		//MGlobal::displayInfo(debugString);
		
		debugString = "RI: ";
		debugString += (std::max(0u, std::min(255u & 0xff, static_cast<uint32_t>(std::floor(fv.x * 256.f)))));
		debugString += " GI: ";
		debugString += (std::max(0u, std::min(255u, static_cast<uint32_t>(std::floor(fv.y * 256.f)))));
		debugString += " BI: ";
		debugString += (std::max(0u, std::min(255u, static_cast<uint32_t>(std::floor(fv.z * 256.f)))));
		//MGlobal::displayInfo(debugString);

		uint32_t RGBA{
			(0xff << 24) +
			(((std::max(0u, std::min(255u, static_cast<uint32_t>(std::floor(fv.z * 256.f))))) & 0xff) << 16) +
			(((std::max(0u, std::min(255u, static_cast<uint32_t>(std::floor(fv.y * 256.f))))) & 0xff) << 8) +
			(((std::max(0u, std::min(255u, static_cast<uint32_t>(std::floor(fv.x * 256.f))))) & 0xff))
		};

		debugString = "RGBA::::: ";
		debugString += RGBA;
		//MGlobal::displayInfo(debugString);

		std::string uuid{ ownerUuid.asChar() };
		size_t uuidSize{ uuid.size() };
		std::vector<char> msg {};
		size_t messageSize {};
		
		msg.resize(
			msg.capacity() +
			STSIZE +
			uuidSize + 
			sizeof(uint32_t)
		);
		memcpy(msg.data(), &uuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
		messageSize += uuidSize;
		memcpy(msg.data() + messageSize, &RGBA, sizeof(uint32_t));
		messageSize += sizeof(uint32_t);

		comlib.addToPackage(
			msg.data(), 
			ComLib::MSG_TYPE::UPDATEVALUES, 
			attribute, 
			messageSize
		);
	}
	/*else if (plug.asMObject().apiType() == MFn::Type::kMatrixData)
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

		comlib.addToPackage(
			msg.data(),
			ComLib::MSG_TYPE::UPDATEVALUES, 
			ComLib::ATTRIBUTE_TYPE::MATRIX, 
			messageSize
		);
	}*/
}
//void pSendPlugConnections(MPlug& plug, MString ownerName, MString ownerUuid, std::string messageType)
//{
//	MPlugArray connectedPlugs;
//	plug.connectedTo(connectedPlugs, true, false);
//	for (UINT i = 0; connectedPlugs.length(); ++i)
//	{
//		MFnDagNode connectedNode(connectedPlugs[i].node());
//		//MGlobal::displayInfo(connectedNode.name());
//	}
//}

//Transform Micro Data
void pSendMatrixData(MObject& object)
{
	MString debugString {};
	MStatus res {};
	MFnDependencyNode node {object};
	MFnTransform transform {object};
	std::string uuid {node.uuid().asString().asChar()};
	size_t uuidSize {uuid.size()};
	
	MGlobal::displayInfo(node.name());
	MFnCamera cam {object};
	MVector forwardVec{cam.viewDirection(MSpace::kWorld)};
	debugString = "For: x:";
	debugString += forwardVec.x;
	debugString += " y: ";
	debugString += forwardVec.y;
	debugString += " z: ";
	debugString += forwardVec.z;
	//MGlobal::displayInfo(debugString);
	
	//
	// Matrix[x][y]
	//
	// Matrix need to be transposed and negated!! D3DMath = row-major | HLSL = column-major!!!
	MMatrix objectMat {transform.transformationMatrix().transpose()};
	double objectMatrix[4][4]{
		objectMat.matrix[0][0],
		objectMat.matrix[0][1],
		objectMat.matrix[0][2]/* * -1.0f*/,
		objectMat.matrix[0][3],

		objectMat.matrix[1][0],
		objectMat.matrix[1][1],
		objectMat.matrix[1][2]/* * -1.0f*/,
		objectMat.matrix[1][3],

		objectMat.matrix[2][0],
		objectMat.matrix[2][1],
		objectMat.matrix[2][2]/* * -1.0f*/,
		objectMat.matrix[2][3],

		objectMat.matrix[3][0],
		objectMat.matrix[3][1],
		objectMat.matrix[3][2]/* * -1.0f*/,
		objectMat.matrix[3][3]
	};
	size_t matrixSize {sizeof(objectMatrix)};
	//pPrintMatrix(objectMatrix);
	
	MMatrix worldMat {objectMat};
	double worldMatrix[4][4] {};
	MFnDependencyNode parentTransformNode {transform.parent(0)};
	if (node.name() == "world")
	{
		worldMat.setToIdentity();

		worldMatrix[0][0] = worldMat.matrix[0][0];
		worldMatrix[0][1] = worldMat.matrix[0][1];
		worldMatrix[0][2] = worldMat.matrix[0][2];
		worldMatrix[0][3] = worldMat.matrix[0][3];

		worldMatrix[1][0] = worldMat.matrix[1][0];
		worldMatrix[1][1] = worldMat.matrix[1][1];
		worldMatrix[1][2] = worldMat.matrix[1][2];
		worldMatrix[1][3] = worldMat.matrix[1][3];

		worldMatrix[2][0] = worldMat.matrix[2][0];
		worldMatrix[2][1] = worldMat.matrix[2][1];
		worldMatrix[2][2] = worldMat.matrix[2][2] * -1.0f;
		worldMatrix[2][3] = worldMat.matrix[2][3];

		worldMatrix[3][0] = worldMat.matrix[3][0];
		worldMatrix[3][1] = worldMat.matrix[3][1];
		worldMatrix[3][2] = worldMat.matrix[3][2];
		worldMatrix[3][3] = worldMat.matrix[3][3];
	}
	else
	{
		//##########################
		//# Calculate world matrix #
		//##########################
		//MDagPath path {};
		//transform.getPath(path);
		//size_t pathCount {path.length()};
		//MGlobal::displayInfo(path.fullPathName());
		//debugString = pathCount;
		//MGlobal::displayInfo(debugString);

		//for (size_t i = 0; i < pathCount; ++i)
		//{
		//	path.pop();
		//	MFnTransform parentTransform {path.node()};
		//	worldMat = parentTransform.transformationMatrix().transpose().operator*(worldMat);
		//}

		MFnTransform parentTransform{ parentTransformNode.object() };
		worldMat = objectMat.operator*(parentTransform.transformationMatrix().transpose());

		worldMatrix[0][0] = worldMat.matrix[0][0];
		worldMatrix[1][0] = worldMat.matrix[1][0];
		worldMatrix[2][0] = worldMat.matrix[2][0]/* * -1.0f*/;
		worldMatrix[3][0] = worldMat.matrix[3][0];

		worldMatrix[0][1] = worldMat.matrix[0][1];
		worldMatrix[1][1] = worldMat.matrix[1][1];
		worldMatrix[2][1] = worldMat.matrix[2][1]/** -1.0f*/;
		worldMatrix[3][1] = worldMat.matrix[3][1];

		worldMatrix[0][2] = worldMat.matrix[0][2];
		worldMatrix[1][2] = worldMat.matrix[1][2];
		worldMatrix[2][2] = worldMat.matrix[2][2]/* * -1.0f*/;
		worldMatrix[3][2] = worldMat.matrix[3][2];

		worldMatrix[0][3] = worldMat.matrix[0][3];
		worldMatrix[1][3] = worldMat.matrix[1][3];
		worldMatrix[2][3] = worldMat.matrix[2][3]/* * -1.0f*/;
		worldMatrix[3][3] = worldMat.matrix[3][3];
	}
	//pPrintMatrix(worldMatrix);

	//MGlobal::displayInfo(";;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;");
	//for (size_t i = 0; i < parentTransformNode.attributeCount(); i++)
	//{
	//	MFnAttribute attr {parentTransformNode.attribute(i)};
	//	debugString = i;
	//	debugString += ": ";
	//	debugString += attr.name().asChar();
	//	MGlobal::displayInfo(debugString);
	//}
	
	// IF INHERITS, SET PARENT TO WORLD (.DAGROOT()), ELSE SET TO PARENT();
	MPlug plug {node.findPlug("inheritsTransform",res)};
	std::string parentUuid{};
	size_t parentUuidSize {};
	if (plug.asBool())
	{
		MFnDependencyNode parentNode {transform.parent(0)};
		parentUuid = parentNode.uuid().asString().asChar();
		parentUuidSize = parentUuid.size();
		//MGlobal::displayInfo(parentNode.name());
	}
	else
	{
		if (node.name() != "world")
		{
			MFnDependencyNode worldNode {transform.dagRoot()};
			parentUuid = worldNode.uuid().asString().asChar();
			parentUuidSize = parentUuid.size();
			//MGlobal::displayInfo(worldNode.name());
		}
	}

	std::vector<char> msg {};
	size_t messageSize {};
	
	msg.resize(
		(STSIZE * 2) + 
		uuidSize +
		(matrixSize * 2) +
		parentUuidSize
	);
	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;
	memcpy(msg.data() + messageSize, &objectMatrix, matrixSize);
	messageSize += matrixSize;
	memcpy(msg.data() + messageSize, &worldMatrix, matrixSize);
	messageSize += matrixSize;
	
	if (node.name() != "world")
	{
		memcpy(msg.data() + messageSize, &parentUuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &parentUuid[0], parentUuidSize);
		messageSize += parentUuidSize;
	}
	comlib.addToPackage(
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
	std::vector<char> msg {};
	size_t messageSize {};

	MFnDependencyNode viewMatrix {cam.parent(0)};
	std::string viewUuid {viewMatrix.uuid().asString().asChar()};
	size_t viewUuidSize {viewUuid.size()};

	double nPlane{ cam.nearClippingPlane() };
	double fPlane{ cam.farClippingPlane() };
	MMatrix projectionMat {cam.projectionMatrix().transpose().matrix};

	// [2][2], [3][2] and [2][3] Controls NDC space!!!
	double projMat[4][4] {};
	if (!cam.isOrtho())
	{
		projMat[0][0] = projectionMat.matrix[0][0];
		projMat[1][0] = projectionMat.matrix[1][0];
		projMat[2][0] = projectionMat.matrix[2][0];
		projMat[3][0] = projectionMat.matrix[3][0];
	
		projMat[0][1] = projectionMat.matrix[0][1];
		projMat[1][1] = projectionMat.matrix[1][1];
		projMat[2][1] = projectionMat.matrix[2][1];
		projMat[3][1] = projectionMat.matrix[3][1];
	
		projMat[0][2] = projectionMat.matrix[0][2];
		projMat[1][2] = projectionMat.matrix[1][2];
		projMat[2][2] = projectionMat.matrix[2][2];
		//-fPlane/(nPlane - fPlane),
		projMat[3][2] = projectionMat.matrix[3][2] * -1.0f;
	
		projMat[0][3] = projectionMat.matrix[0][3];
		projMat[1][3] = projectionMat.matrix[1][3];
		//(nPlane*fPlane) / (nPlane - fPlane),
		projMat[2][3] = projectionMat.matrix[2][3] * -1.0f;
		projMat[3][3] = projectionMat.matrix[3][3];

	}
	else
	{
		projMat[0][0] = projectionMat.matrix[0][0];
		projMat[1][0] = projectionMat.matrix[1][0];
		projMat[2][0] = projectionMat.matrix[2][0];
		projMat[3][0] = projectionMat.matrix[3][0];

		projMat[0][1] = projectionMat.matrix[0][1];
		projMat[1][1] = projectionMat.matrix[1][1];
		projMat[2][1] = projectionMat.matrix[2][1];
		projMat[3][1] = projectionMat.matrix[3][1];

		projMat[0][2] = projectionMat.matrix[0][2];
		projMat[1][2] = projectionMat.matrix[1][2];
		
		//projMat[2][2] = projectionMat.matrix[2][2];
		projMat[2][2] = 1 / (fPlane - nPlane);
		//projMat[2][2] = (nPlane) / (fPlane - nPlane);

		projMat[3][2] = projectionMat.matrix[3][2] * 1.0f;

		projMat[0][3] = projectionMat.matrix[0][3];
		projMat[1][3] = projectionMat.matrix[1][3];
		
		//projMat[2][3] = projectionMat.matrix[2][3] * -1.0f;
		projMat[2][3] = (nPlane) / (fPlane - nPlane);
		//projMat[2][3] = 1 / (fPlane - nPlane);

		projMat[3][3] = projectionMat.matrix[3][3];
	}

	//MGlobal::displayInfo("############ Perspective: ");
	//pPrintMatrix(projMat);

	msg.resize(
		(STSIZE * 2) +
		uuidSize +
		sizeof(double[4][4]) +
		viewUuidSize
	);
	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;

	memcpy(msg.data() + messageSize, &projMat, sizeof(double[4][4]));
	messageSize += sizeof(double[4][4]);

	memcpy(msg.data() + messageSize, &viewUuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &viewUuid[0], viewUuidSize);
	messageSize += viewUuidSize;

	comlib.addToPackage(
		msg.data(), 
		ComLib::MSG_TYPE::UPDATEVALUES, 
		ComLib::ATTRIBUTE_TYPE::PROJMATRIX, 
		messageSize
	);
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
//	comlib.addToPackage(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::VERTEX, messageSize);
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
//			comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEX, messageSize);
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
//		comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEX, messageSize);
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
//	comlib.addToPackage(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::NORMAL, messageSize);
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
//			comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMAL, messageSize);
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
//		comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMAL, messageSize);
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
//		comlib.addToPackage(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::UVSETS, messageSize);
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
//			comlib.addToPackage(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::UVSET, messageSize);
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
//					comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UV, messageSize);
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
//				comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UV, messageSize);
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
//			comlib.addToPackage(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::UVID, messageSize);
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
//	//				//comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UVID, messageSize);
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
//	//			//comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::UVID, messageSize);
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
//	////comlib.addToPackage(msg, ComLib::MSG_TYPE::ALLOCATE, ComLib::ATTRIBUTE_TYPE::VERTEXID, messageSize);
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
//	////comlib.addToPackage(msg, ComLib::MSG_TYPE::ALLOCATE , ComLib::ATTRIBUTE_TYPE::NORMALID, messageSize);
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
//			//comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEXID, messageSize);
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
//		//comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::VERTEXID, messageSize);
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
//			//comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMALID, messageSize);
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
//		//comlib.addToPackage(msg, ComLib::MSG_TYPE::UPDATEVALUES, ComLib::ATTRIBUTE_TYPE::NORMALID, messageSize);
//		//msg.Reset();
//		//messageSize = 0;
//		counter = -1;
//	}
//	MGlobal::displayInfo("IDs done!");
//}

void pSendVertexData(MObject& object, bool allocate)  
{
	MStatus res {};
	MString debugString {};
	struct VERTEX
	{
		float point[3]		{};
		float normal[3]		{};
		float tangent[3]	{};
		float bitangent[3]	{};
		float uv[2]			{};
	};
	std::vector<VERTEX> vertexList	{};
	std::vector<UINT32> vertexIDs	{};
	vertexList.resize(100);
	vertexIDs.resize(200);
	int counter  {-1};
	int counter2 {-1};
	MFnMesh mesh{ object };
	std::string uuid{mesh.uuid().asString().asChar()};
	size_t uuidSize{uuid.size()};
	
	std::vector<char> msg	{};
	size_t messageSize		{};
	std::vector<char> msg2	{};
	size_t messageSize2		{};

	// Object-Relative vertex positions. 8 for a cube.
	MPointArray vertexLookUpTable{};
	mesh.getPoints(vertexLookUpTable);
	
	MGlobal::displayInfo("#########################");
	MGlobal::displayInfo("Mesh Points: ");
	for (size_t i = 0; i < vertexLookUpTable.length(); ++i)
	{
		debugString = "Vertex: ";
		debugString += vertexLookUpTable[i].x;
		debugString += " , ";
		debugString += vertexLookUpTable[i].y;
		debugString += " , ";
		debugString += vertexLookUpTable[i].z;
		MGlobal::displayInfo(debugString);
	}
	MGlobal::displayInfo("#########################");

	MStringArray uvSetNames{};
	mesh.getUVSetNames(uvSetNames);

	MFloatVectorArray normalLookup{};
	mesh.getNormals(normalLookup, MSpace::kObject);
	/*debugString = "Object Normal length";
	debugString += normalLookup.length();
	MGlobal::displayInfo(debugString);
	for (size_t i = 0; i < normalLookup.length(); ++i)
	{
		debugString = "R: ";
		debugString += normalLookup[i].x;
		debugString += " G: ";
		debugString += normalLookup[i].y;
		debugString += " B: ";
		debugString += normalLookup[i].z;
		MGlobal::displayInfo(debugString);
	}*/

	MFloatVectorArray tangentLookup{};
	mesh.getTangents(tangentLookup, MSpace::kObject, &uvSetNames[0]);
	/*debugString = "Tangent Normal length";
	debugString += tangentLookup.length();
	MGlobal::displayInfo(debugString);
	for (size_t i = 0; i < tangentLookup.length(); ++i)
	{
		debugString = "R: ";
		debugString += tangentLookup[i].x;
		debugString += " G: ";
		debugString += tangentLookup[i].y;
		debugString += " B: ";
		debugString += tangentLookup[i].z;
		MGlobal::displayInfo(debugString);
	}*/

	MFloatVectorArray bitangentLookup{};
	mesh.getBinormals(bitangentLookup, MSpace::kObject, &uvSetNames[0]);
	/*debugString = "Bitangent Normal length";
	debugString += bitangentLookup.length();
	MGlobal::displayInfo(debugString);
	for (size_t i = 0; i < bitangentLookup.length(); ++i)
	{
		debugString = "R: ";
		debugString += bitangentLookup[i].x;
		debugString += " G: ";
		debugString += bitangentLookup[i].y;
		debugString += " B: ";
		debugString += bitangentLookup[i].z;
		MGlobal::displayInfo(debugString);
	}*/

	// UV Space UVs -> 14 for a cube
	MFloatArray U, V;
	mesh.getUVs(U, V, &uvSetNames[0]);
	/*MGlobal::displayInfo("UV count");
	debugString = U.length();
	MGlobal::displayInfo(debugString);
	for (size_t i = 0; i < U.length(); ++i)
	{
		debugString = "U: ";
		debugString += U[i];
		debugString += " | V: ";
		debugString += V[i];
		MGlobal::displayInfo(debugString);
	}*/

	MItMeshPolygon faceIt(object, &res);
	if (res == MS::kSuccess)
	{
		//####################################################
		//####################################################
		//#					FACES ALLOCATION				 #
		//####################################################
		//####################################################

		//if (allocate)
		//{
			msg.resize(
				STSIZE +
				uuidSize +
				sizeof(UINT)
			);
			memcpy(msg.data(), &uuidSize, STSIZE);
			messageSize += STSIZE;
			memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
			messageSize += uuidSize;
			
			UINT faceCount {faceIt.count()};
			memcpy(msg.data() + messageSize, &faceCount, sizeof(UINT));
			messageSize += sizeof(UINT);
			
			comlib.addToPackage(
				msg.data(),
				ComLib::MSG_TYPE::ALLOCATE,
				ComLib::ATTRIBUTE_TYPE::FACES,
				messageSize
			);
			msg.clear();
			messageSize = 0;
		//}

		for (; !faceIt.isDone(); faceIt.next())
		{
			UINT faceIndex {faceIt.index()};
			bool triangulated {};
			//MGlobal::displayInfo(debugString);

			// Lists points and IDs from Object-Relative face triangles.
			// Multiple copies of same points. (Refer to mesh.getPoints() list above instead to clean up duplicates.
			// IDs are Object-Relative, shared normals and uvs -> on a cube, 1 UV/Normal vector per vertex instead of 3(per-face).
			// See for loop below for fetching Face-Relative vertex IDs.
			MPointArray faceVertexTriangleList			{}; // Not needed
			MIntArray  objectRelativeTriangleIDs		{};
			faceIt.getTriangles(faceVertexTriangleList, objectRelativeTriangleIDs, MSpace::kObject);
			
			MGlobal::displayInfo("#########################");
			debugString = "Tris ID";
			debugString += ": ";
			MGlobal::displayInfo(debugString);
			for (size_t i = 0; i < objectRelativeTriangleIDs.length(); ++i)
			{
				debugString = "ID: ";
				debugString += objectRelativeTriangleIDs[i];
				MGlobal::displayInfo(debugString);
			}
			MGlobal::displayInfo("#########################");

			debugString = faceVertexTriangleList.length();
			//MGlobal::displayInfo(debugString);
			debugString = (objectRelativeTriangleIDs.length());
			//MGlobal::displayInfo(debugString);
			
			if (faceVertexTriangleList.length() == 3)
			{
				//MGlobal::displayInfo("Triangulated!");
				triangulated = true;
			}
			// Length is still Face-Relative and will give correct per-face ID drawCount.
			UINT faceIDCount {objectRelativeTriangleIDs.length()};

			MPointArray facePointList {};
			faceIt.getPoints(facePointList);
			UINT faceVertexCount {facePointList.length()};
			
			MGlobal::displayInfo("#########################");
			debugString = "Face Point ";
			debugString += faceIndex;
			debugString += ":";
			MGlobal::displayInfo(debugString);
			for (size_t i = 0; i < facePointList.length(); ++i)
			{
				debugString = "Vertex: ";
				debugString += facePointList[i].x;
				debugString += " , ";
				debugString += facePointList[i].y;
				debugString += " , ";
				debugString += facePointList[i].z;
				MGlobal::displayInfo(debugString);
			}
			MGlobal::displayInfo("#########################");

			MVectorArray norms {};
			faceIt.getNormals(norms, MSpace::kObject);
			//debugString = "Face Normal length";
			//debugString += norms.length();
			//MGlobal::displayInfo(debugString);
			//for (size_t i = 0; i < norms.length(); ++i)
			//{
			//	debugString = "R: ";
			//	debugString += norms[i].x;
			//	debugString += " G: ";
			//	debugString += norms[i].y;
			//	debugString += " B: ";
			//	debugString += norms[i].z;
			//	MGlobal::displayInfo(debugString);
			//}

			//####################################################
			//####################################################
			//#					VERTEX ALLOCATION				 #
			//####################################################
			//####################################################
			//if (allocate)
			//{
				msg.resize(
					STSIZE +
					uuidSize +
					(sizeof(UINT) * 3)
				);
				memcpy(msg.data(), &uuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
				messageSize += uuidSize;
				
				memcpy(msg.data() + messageSize, &faceIndex, sizeof(UINT));
				messageSize += sizeof(UINT);
				memcpy(msg.data() + messageSize, &faceVertexCount, sizeof(UINT));
				messageSize += sizeof(UINT);
				memcpy(msg.data() + messageSize, &faceIDCount, sizeof(UINT));
				messageSize += sizeof(UINT);
				
				comlib.addToPackage(
					msg.data(),
					ComLib::MSG_TYPE::ALLOCATE,
					ComLib::ATTRIBUTE_TYPE::VERTEX,
					messageSize
				);
				msg.clear();
				messageSize = 0;
			//}
			
			//###################################################
			//###################################################
			//#				VERTEXLIST/ID  QUERY				# 
			//###################################################
			//###################################################

			// Vertex List per face. 
			// (Since Object-Relativity does not consider Face-Relative UVs/Normals.)
			// (DX11 index buffers only allow one index list, Splitting vertex buffers still result in "draw / Object-Points")
			// (Since drawcalls are Face-Relative, mismatch with Object-Relative points and Face-Relative UVs/Normals)
			// Face-Relative gives more drawcalls than Object-Relative, but less than Per-Vertex drawing (If model is not triangulated).
			// Face-Relativity allows creation of "Face Shells".
			msg.resize(
				STSIZE +
				uuidSize +
				(sizeof(UINT) * 2) +
				sizeof(int) +
				sizeof(VERTEX) * 100
			);
			// Converts Object-Relative IDs to Face-Relative IDs
			msg2.resize(
				STSIZE +
				uuidSize +
				(sizeof(UINT) * 2) +
				sizeof(int) +
				sizeof(UINT32) * 200
			);
			//MGlobal::displayInfo("-----------");

			// Walk through the face vertex ID list (triangle sorted)(0-n).
			//counter == pointCounter | counter2 == IDCounter
			for (UINT i = 0; i < faceIDCount; ++i)
			{
				//debugString = "FACE ID: ";
				//debugString += (UINT)i;
				//MGlobal::displayInfo(debugString);
				
				//Walk through the local object-space face points.
				for (UINT j = 0; j < faceVertexCount; ++j)
				{
					// Compare with global world-space vertex list.
					if (vertexLookUpTable[objectRelativeTriangleIDs[i]].x == facePointList[j].x &&
						vertexLookUpTable[objectRelativeTriangleIDs[i]].y == facePointList[j].y &&
						vertexLookUpTable[objectRelativeTriangleIDs[i]].z == facePointList[j].z	  )
					{
						counter2++;
						int pointIndex {static_cast<int>(j)};
						//debugString = "counter2: ";
						//debugString += counter2;
						//MGlobal::displayInfo(debugString);

						// Used to correct winding order for Dx11
						if (triangulated)
						{
							//MGlobal::displayInfo("TRUE!");
							//Triangle List
							if ((counter2 % 6) == 1)
							{
								vertexIDs[counter2] = j + 1;
								//pointIndex = 2;
							}
							else if ((counter2 % 6) == 2)
							{
								vertexIDs[counter2] = j - 1;
								//pointIndex = 2;
							}
							else {
								vertexIDs[counter2] = j;
							}
						}
						else
						{
							//MGlobal::displayInfo("FALSE!");
							//Triangle Strip
							//vertexIDs[counter2] = j;
							//if ((counter2 % 6) == 2 || (counter2 % 6) == 3)
							//{
							//	vertexIDs[counter2] = j-1;
							//	//pointIndex = j-1;
							//}
							//else if ((counter2 % 6) == 5)
							//{
							//	vertexIDs[counter2] = j+1;
							//	//pointIndex = j+1;
							//}
							//else
							//{
							//	vertexIDs[counter2] = j;
							//	//pointIndex = j;
							//}
							//if ((counter2 % 6) == 1 || (counter2 % 6) == 4|| (counter2 % 6) == 5)
							//{
							//	vertexIDs[counter2] = j + 1;
							//}
							//else if ((counter2 % 6) == 2)
							//{
							//	vertexIDs[counter2] = j - 2;
							//}
							//else if ((counter2 % 6) == 3)
							//{
							//	vertexIDs[counter2] = j - 2;
							//}
							//else
							//{
							//	vertexIDs[counter2] = j;
							//}
							if ((counter2 % 6) == 1 || (counter2 % 6) == 4)
							{
								vertexIDs[counter2] = 1;
								//pointIndex = 3;
							}
							else if ((counter2 % 6) == 2 || ((counter2 % 6) == 3))
							{
								vertexIDs[counter2] = 2;
								//pointIndex = 1;
							}
							else if ((counter2 % 6) == 5)
							{
								vertexIDs[counter2] = 3;
								//pointIndex = 2;
							}
							else
							{
								vertexIDs[counter2] = 0;
								//pointIndex = 0;
							}
						}

						//debugString = "VERTEX ID: ";
						////debugString += (UINT)j;
						//debugString += vertexIDs[counter2];
						//MGlobal::displayInfo(debugString);
						
						//Ensure we are not saving duplicate vertex points (Think of more efficient way)
						if ((i % 6) != 3 && (i % 6) != 4)
						{
							counter++;
							
							vertexList[counter].point[0] = static_cast<float>(facePointList[pointIndex].x);
							vertexList[counter].point[1] = static_cast<float>(facePointList[pointIndex].y);
							vertexList[counter].point[2] = static_cast<float>(facePointList[pointIndex].z);

							//debugString = "X: ";
							//debugString += vertexList[counter].point[0];
							//debugString += " Y: ";
							//debugString += vertexList[counter].point[1];
							//debugString += " Z: ";
							//debugString += vertexList[counter].point[2];
							//MGlobal::displayInfo(debugString);

							MVector faceVertNormal{};
							faceIt.getNormal(vertexIDs[counter2], faceVertNormal, MSpace::kObject);
							vertexList[counter].normal[0] = static_cast<float>(faceVertNormal.x);
							vertexList[counter].normal[1] = static_cast<float>(faceVertNormal.y);
							vertexList[counter].normal[2] = static_cast<float>(faceVertNormal.z);

							/*debugString = "R: ";
							debugString += vertexList[counter].normal[0];
							debugString += " G: ";
							debugString += vertexList[counter].normal[1];
							debugString += " B: ";
							debugString += vertexList[counter].normal[2];
							MGlobal::displayInfo(debugString);*/

							MVector tangentVector {};
							UINT tangentIndex {faceIt.tangentIndex(vertexIDs[counter2])};
							vertexList[counter].tangent[0] = static_cast<float>(tangentLookup[tangentIndex].x);
							vertexList[counter].tangent[1] = static_cast<float>(tangentLookup[tangentIndex].y);
							vertexList[counter].tangent[2] = static_cast<float>(tangentLookup[tangentIndex].z);

							vertexList[counter].bitangent[0] = static_cast<float>(bitangentLookup[tangentIndex].x);
							vertexList[counter].bitangent[1] = static_cast<float>(bitangentLookup[tangentIndex].y);
							vertexList[counter].bitangent[2] = static_cast<float>(bitangentLookup[tangentIndex].z);

							/*debugString = "TR: ";
							debugString += vertexList[counter].tangent[0];
							debugString += " TG: ";
							debugString += vertexList[counter].tangent[1];
							debugString += " TB: ";
							debugString += vertexList[counter].tangent[2];
							MGlobal::displayInfo(debugString);

							debugString = "BR: ";
							debugString += vertexList[counter].bitangent[0];
							debugString += " BG: ";
							debugString += vertexList[counter].bitangent[1];
							debugString += " BB: ";
							debugString += vertexList[counter].bitangent[2];
							MGlobal::displayInfo(debugString);*/

							int uvIndex{};
							faceIt.getUVIndex(static_cast<int>(pointIndex), uvIndex, &uvSetNames[0]);
							vertexList[counter].uv[0] = static_cast<float>(U[uvIndex]);
							vertexList[counter].uv[1] = static_cast<float>(V[uvIndex]);
				
							/*debugString = "U: ";
							debugString += vertexList[counter].uv[0];
							debugString += " V: ";
							debugString += vertexList[counter].uv[1];
							MGlobal::displayInfo(debugString);*/

							//MGlobal::displayInfo("-----------");
					
							if (counter % 99 == 0 && counter != 0)
							{
								memcpy(msg.data(), &uuidSize, STSIZE);
								messageSize += STSIZE;
								memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
								messageSize += uuidSize;

								memcpy(msg.data() + messageSize, &faceIndex, sizeof(UINT));
								messageSize += sizeof(UINT);
								memcpy(msg.data() + messageSize, &i, sizeof(UINT));
								messageSize += sizeof(UINT);
								memcpy(msg.data() + messageSize, &counter, sizeof(int));
								messageSize += sizeof(int);
								memcpy(msg.data() + messageSize, vertexList.data(), sizeof(VERTEX) * static_cast<size_t>(counter + 1));
								messageSize += sizeof(VERTEX) * static_cast<size_t>(counter + 1);

								comlib.addToPackage(
									msg.data(),
									ComLib::MSG_TYPE::UPDATEVALUES,
									ComLib::ATTRIBUTE_TYPE::VERTEX,
									messageSize
								);
								messageSize = 0;
								counter = -1;
							}
						}
						if (counter2 % 199 == 0 && counter2 != 0)
						{
							memcpy(msg2.data(), &uuidSize, STSIZE);
							messageSize2 += STSIZE;
							memcpy(msg2.data() + messageSize2, &uuid[0], uuidSize);
							messageSize2 += uuidSize;

							memcpy(msg2.data() + messageSize2, &faceIndex, sizeof(UINT));
							messageSize2 += sizeof(UINT);
							memcpy(msg2.data() + messageSize2, &i, sizeof(UINT));
							messageSize2 += sizeof(UINT);
							memcpy(msg2.data() + messageSize2, &counter2, sizeof(int));
							messageSize2 += sizeof(int);
							memcpy(msg2.data() + messageSize2, vertexIDs.data(), sizeof(UINT32) * static_cast<size_t>(counter2 + 1));
							messageSize2 += sizeof(UINT32) * static_cast<size_t>(counter2 + 1);

							comlib.addToPackage(
								msg2.data(),
								ComLib::MSG_TYPE::UPDATEVALUES,
								ComLib::ATTRIBUTE_TYPE::VERTEXID,
								messageSize2
							);
							messageSize2 = 0;
							counter2 = -1;
						}
					}
				}
			}
			if (counter >= 0)
			{
				memcpy(msg.data(), &uuidSize, STSIZE);
				messageSize += STSIZE;
				memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
				messageSize += uuidSize;

				memcpy(msg.data() + messageSize, &faceIndex, sizeof(UINT));
				messageSize += sizeof(UINT);
				memcpy(msg.data() + messageSize, &faceVertexCount, sizeof(UINT));
				messageSize += sizeof(UINT);
				memcpy(msg.data() + messageSize, &counter, sizeof(int));
				messageSize += sizeof(int);

				memcpy(msg.data() + messageSize, vertexList.data(), (sizeof(VERTEX) * static_cast<size_t>(counter + 1)));
				messageSize += (sizeof(VERTEX) * static_cast<size_t>(counter + 1));

				comlib.addToPackage(
					msg.data(),
					ComLib::MSG_TYPE::UPDATEVALUES,
					ComLib::ATTRIBUTE_TYPE::VERTEX,
					messageSize
				);
				msg.clear();
				messageSize = 0;
				counter = -1;
			}
			if (counter2 >= 0)
			{
				memcpy(msg2.data(), &uuidSize, STSIZE);
				messageSize2 += STSIZE;
				memcpy(msg2.data() + messageSize2, &uuid[0], uuidSize);
				messageSize2 += uuidSize;

				memcpy(msg2.data() + messageSize2, &faceIndex, sizeof(UINT));
				messageSize2 += sizeof(UINT);
				memcpy(msg2.data() + messageSize2, &faceIDCount, sizeof(UINT));
				messageSize2 += sizeof(UINT);
				memcpy(msg2.data() + messageSize2, &counter2, sizeof(int));
				messageSize2 += sizeof(int);
				memcpy(msg2.data() + messageSize2, vertexIDs.data(), sizeof(UINT32) * static_cast<size_t>(counter2 + 1));
				messageSize2 += sizeof(UINT32) * static_cast<size_t>(counter2 + 1);

				comlib.addToPackage(
					msg2.data(),
					ComLib::MSG_TYPE::UPDATEVALUES,
					ComLib::ATTRIBUTE_TYPE::VERTEXID,
					messageSize2
				);
				messageSize2 = 0;
				counter2 = -1;
			}
			//MGlobal::displayInfo("####################################################");
		}
	}
	else
	{
		MGlobal::displayInfo("Error fetching face information!");
	}
}

//Material Micro Data
void pSendConnectedShader(MObject& object)
{
	MStatus res {};
	MString debugString {};
	MFnMesh mesh {object};
	std::string uuid {mesh.uuid().asString().asChar()};
	size_t uuidSize {uuid.size()};

	MObjectArray shaderEngineArray {};
	MIntArray faceIndiciesArray {};
	mesh.getConnectedShaders(0, shaderEngineArray, faceIndiciesArray);
	size_t shaderCount {shaderEngineArray.length()};
	
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

	std::vector<char> msg{};
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

	comlib.addToPackage(
		msg.data(),
		ComLib::MSG_TYPE::UPDATEVALUES, 
		ComLib::ATTRIBUTE_TYPE::MESHSHADERS,
		messageSize
	);
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
		//MGlobal::displayInfo("DAG");
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

		comlib.addToPackage(
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
		comlib.addToPackage(
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

	std::vector<char> msg {};
	size_t messageSize = 0;

	memcpy(msg.data(), &targetUuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &targetUuid[0], targetUuidSize);
	messageSize += targetUuidSize;

	size_t parentCount = dag.parentCount();
	memcpy(msg.data() + messageSize, &parentCount, STSIZE);
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

	comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::ADDVALUES, ComLib::ATTRIBUTE_TYPE::PARENT, messageSize);
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

	MFnDependencyNode parentNode(mesh.parent(0));
	std::string transformUuid {parentNode.uuid().asString().asChar()};
	size_t transformUuidSize {transformUuid.size()};
	
	std::vector<char> msg {};
	size_t messageSize {};
	msg.resize(
		(STSIZE * 2) +
		uuidSize +
		transformUuidSize
	);

	memcpy(msg.data() + messageSize, &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;

	memcpy(msg.data() + messageSize, &transformUuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &transformUuid[0], transformUuidSize);
	messageSize += transformUuidSize;

	comlib.addToPackage(
		msg.data(),
		ComLib::MSG_TYPE::UPDATEVALUES,
		ComLib::ATTRIBUTE_TYPE::MESHTRANSFORMER,
		messageSize
	);

	// Will not be sent individually due to attribute callbacks.
	// Look for possible solution for separation in the future.
	pSendVertexData(object, 1);
	//pSendVertexData2(object);


	pSendConnectedShader(object);
}
void pSendCameraData(MObject& object)
{
	pSendProjectionMatrix(object);
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
	MFnDependencyNode transform {pointLightNode.parent(0)};
	std::string transformUuid {transform.uuid().asString().asChar()};
	size_t transformUuidSize {transformUuid.size()};

	debugString = pointLightNode.decayRate();
	//MGlobal::displayInfo(debugString);
	std::vector<char> msg {};
	size_t messageSize {};
	
	float intensity {pointLightNode.intensity()};
	float color[3] { 
		pointLightNode.color().r, 
		pointLightNode.color().g,
		pointLightNode.color().b 
	};
	
	msg.resize(
		(STSIZE * 2) +
		uuidSize +
		transformUuidSize + 
		sizeof(float) +
		sizeof(float[3])
	);
	
	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;
	
	memcpy(msg.data() + messageSize, &transformUuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &transformUuid[0], transformUuidSize);
	messageSize += transformUuidSize;
	
	memcpy(msg.data() + messageSize, &intensity, sizeof(float));
	messageSize += sizeof(float);
	memcpy(msg.data() + messageSize, &color, sizeof(color));
	messageSize += sizeof(color);
	
	comlib.addToPackage(
		msg.data(),
		ComLib::MSG_TYPE::UPDATEVALUES,
		ComLib::ATTRIBUTE_TYPE::POINTINTENSITY,
		messageSize
	);
	//MGlobal::displayInfo("7");
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

			comlib.addToPackage(
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

		comlib.addToPackage(
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
		MMaterial mats {materialNode.object()};
		MColor color;
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

				comlib.addToPackage(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::SHCOLORMAP,
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s color is not connected!";
				//MGlobal::displayInfo(debugString);
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

				comlib.addToPackage(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::SHAMBIENTCOLORMAP, 
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s ambientColor is not connected!";
				//MGlobal::displayInfo(debugString);
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

				comlib.addToPackage(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::SHTRANSPARANCYMAP,
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s transparency is not connected!";
				//MGlobal::displayInfo(debugString);
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
					comlib.addToPackage(
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

				comlib.addToPackage(
					msg.data(), 
					ComLib::MSG_TYPE::UPDATEVALUES, 
					ComLib::ATTRIBUTE_TYPE::SHNORMALMAP, 
					messageSize);
			}
			else
			{
				debugString = materialUuid;
				debugString += "'s normalCamera is not connected!";
				//MGlobal::displayInfo(debugString);
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

		comlib.addToPackage(
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
		//MGlobal::displayInfo(debugString);
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
	//comlib.addToPackage(msg, ComLib::MSG_TYPE::RENAME, ComLib::ATTRIBUTE_TYPE::NODENAME, messageSize);
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
	//comlib.addToPackage(msg, ComLib::MSG_TYPE::RENAME, ComLib::ATTRIBUTE_TYPE::NODEUUID, messageSize);
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
	//MGlobal::displayInfo(debugString);
	
	debugString = "ATTRIBUTE NAME: ";
	debugString += attributeName;
	//MGlobal::displayInfo(debugString);

	//MGlobal::displayInfo("##########  ATTRIBUTE ADDED #########");
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
	//MGlobal::displayInfo(debugString);
	
	debugString = "ATTRIBUTE NAME: ";
	debugString += attributeName;
	//MGlobal::displayInfo(debugString);

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
				pSendMeshData(nodeObject);
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
		//MGlobal::displayInfo(debugString);

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
		std::vector<char> msg {};
		size_t messageSize {};

		msg.reserve(
			(STSIZE * 2) + 
			targetUuidSize +
			parentUuidSize
		);

		memcpy(msg.data(), &targetUuidSize, STSIZE);
		messageSize += STSIZE; 
		memcpy(msg.data() + messageSize, &targetUuid[0], targetUuidSize);
		messageSize += targetUuidSize;

		memcpy(msg.data() + messageSize, &parentUuidSize, STSIZE);
		messageSize += STSIZE;

		memcpy(msg.data() + messageSize, &parentUuid[0], parentUuidSize);
		messageSize += parentUuidSize;

		comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::ADDVALUES, ComLib::ATTRIBUTE_TYPE::PARENT, messageSize);
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
		std::vector<char> msg {};
		size_t messageSize {};
		
		msg.resize(
			(STSIZE * 2) +
			targetUuidSize + 
			parentUuidSize
		);

		memcpy(msg.data(), &targetUuidSize, STSIZE);
		messageSize += STSIZE; 
		memcpy(msg.data() + messageSize, &targetUuid[0], targetUuidSize);
		messageSize += targetUuidSize;
		
		memcpy(msg.data() + messageSize, &parentUuidSize, STSIZE);
		messageSize += STSIZE;
		memcpy(msg.data() + messageSize, &parentUuid[0], parentUuidSize);
		messageSize += parentUuidSize;
		
		comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::REMOVEVALUES, ComLib::ATTRIBUTE_TYPE::PARENT, messageSize);
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
	//MCallbackId AttributeAddedRemovedID = MNodeMessage::addAttributeAddedOrRemovedCallback(
	//object,
	//pAttributeAddedRemovedCallback,
	//NULL,
	//&res);
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(AttributeAddedRemovedID) == MS::kSuccess) {};
	//}
}
void pRemoveNodeCallbacks(MObject& object)
{
	MCallbackIdArray nodeCallbackIds {};
	MMessage::nodeCallbacks(object, nodeCallbackIds);

	for (size_t i = 0; i < myCallbackArray.length(); ++i)
	{
		for (size_t j = 0; j < nodeCallbackIds.length(); ++j)
		{
			if (myCallbackArray[i] == nodeCallbackIds[j])
			{
				myCallbackArray.remove(i);
			}
		}
	}
	MMessage::removeCallbacks(nodeCallbackIds);
}

void pNodeCreationCallback(MObject& object, void* clientData)
{
	MStatus res;
	MString debugString;
	MFnDependencyNode node(object);

	//MGlobal::displayInfo(node.typeName());

	switch (object.apiType())
	{
	case MFn::kTransform:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendTransformData(object);
	case MFn::kMesh:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		break;
	case MFn::kCamera:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendCameraData(object);
		break;
	case MFn::kPointLight:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		break;
	case MFn::kShadingEngine:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendShaderGroupData(object);
		break;
		// Surface Materials
	case MFn::kLambert:
	case MFn::kBlinn:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendMaterialData(object);
		break;
	//case MFn::kPhong:
	//case MFn::kPhongExplorer:
	//case MFn::kLayeredShader:
	//case MFn::kRampShader:
	//case MFn::kShadingMap:
	//case MFn::kHairTubeShader:
		////Odd Materials
		//case MFn::kSurfaceShader:
		//case MFn::kVolumeShader:
		//	break;
		//// Custom Materials (custom, StingrayPBR, DirectX
		//case MFn::kPluginHardwareShader:
		//case MFn::kPluginDependNode:
		//	//Not yet supported
		//	break;
		//
		//// Utils Material
		//case MFn::kUseBackground:
		//case MFn::kAnisotropy:
		//case MFn::kEnvFogMaterial:
		//case MFn::kFluid:
		//case MFn::kLightFogMaterial:
		//case MFn::kOceanShader:
		//case MFn::kDisplacementShader:
		//case MFn::kBulge:
		//case MFn::kChecker:
		//case MFn::kCloth:
		//case MFn::kFluidTexture2D:

		//	break;
	case MFn::kFileTexture:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendTextureData(object);
		break;
	default:
		break;
	}
}
void pNodeDeleteCallback(MObject& object, void* clientData)
{
	MStatus res {};
	MString debugString {};
	MFnDependencyNode node {object};
	std::string uuid {node.uuid().asString().asChar()};
	size_t uuidSize {uuid.size()};
	
	std::vector<char> msg {};
	size_t messageSize {};

	pRemoveNodeCallbacks(object);

	msg.resize(
		STSIZE +
		uuidSize
	);

	memcpy(msg.data(), &uuidSize, STSIZE);
	messageSize += STSIZE;
	memcpy(msg.data() + messageSize, &uuid[0], uuidSize);
	messageSize += uuidSize;
	
	comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::DEALLOCATE, ComLib::ATTRIBUTE_TYPE::NODE, messageSize);
}

//void pGetChildParenting(MObject& currentObject)
//{
//	MString debugString;
//	MStatus res;
//
//	MFnDagNode currentDag(currentObject);
//
//	for (UINT i = 0; i < currentDag.childCount(); ++i)
//	{
//		MObject childObject(currentDag.child(i));
//		MFnDagNode childDAG(childObject);
//
//		switch (childObject.apiType())
//		{
//		case MFn::kTransform:
//			pSendParentsData(childObject);
//			break;
//		case MFn::kMesh:
//		case MFn::kCamera:
//		case MFn::kPointLight:
//		default:
//			break;
//		}
//
//		if (childDAG.childCount() > 0)
//		{
//			pGetChildParenting(childObject);
//		}
//	}
//
//}
//void pGetParentChildRelationship()
//{
//	MString debugString {};
//	MStatus res;
//
//	//ComPtr<char> msg {};
//	size_t messageSize = 0;
//
//	//comlib.addToPackage(msg.Get(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::EXSTART, messageSize);
//
//	//Queries the current viewport.
//	M3dView sceneView;
//	sceneView = sceneView.active3dView();
//
//	//Queries the active cameras DagPath.
//	MDagPath camShapeDagPath;
//	sceneView.getCamera(camShapeDagPath);
//
//	//Get root node
//	MFnDagNode camDAG(camShapeDagPath.node());
//	MFnDagNode rootDAG(camDAG.dagRoot());
//
//	for (UINT i = 0; i < rootDAG.childCount(); ++i)
//	{
//		MFnDagNode rootChildDAG(rootDAG.child(i));
//		if (rootChildDAG.typeName() == "transform")
//		{
//			MObject rootChildObject(rootChildDAG.object());
//			MString rootChildName(rootChildDAG.name());
//
//			//Hard-coded. Possible automation
//			//Filter internal essencial scene objects, carefull when modifying
//			if (rootChildName != "groundPlane_transform"
//				&& rootChildName != "defaultUfeProxyParent"
//				&& rootChildName != "shaderBallCamera1"
//				&& rootChildName != "shaderBallOrthoCamera1"
//				&& rootChildName != "MayaMtlView_FillLight1"
//				&& rootChildName != "MayaMtlView_RimLight1")
//			{
//				pSendParentsData(rootChildObject);
//
//				if (rootChildDAG.childCount())
//				{
//					pGetChildParenting(rootChildObject);
//				}
//			}
//		}
//	}
//}

void pCreateAddCallbackChildNode(MObject& currentObject)
{
	MString debugString;
	MStatus res;

	MFnDagNode currentDag(currentObject);
	//MGlobal::displayInfo("Child Added");
	for (UINT i = 0; i < currentDag.childCount(); ++i)
	{
		MObject childObject(currentDag.child(i));
		MFnDagNode childDAG(childObject);

		switch (childObject.apiType())
		{
		case MFn::kTransform:
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendTransformData(childObject);
			break;
		case MFn::kMesh:
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendMeshData(childObject);
			break;
		case MFn::kCamera:
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendCameraData(childObject);
			break;
		case MFn::kPointLight:
			//MGlobal::displayInfo("PointLight!");
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendPointLightData(childObject);
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
		pAddNodeCallbacks(textureObject);
		pAllocNode(textureObject);
		pSendTextureData(textureObject);
	}

	//Iterate through all bump nodes (Unnecessary node atm? Maya internal system, extra step.)
	MItDependencyNodes bumpIt(MFn::kBump);
	for (; !bumpIt.isDone(); bumpIt.next())
	{
		MObject bumpObject(bumpIt.thisNode());
		pAddNodeCallbacks(bumpObject);
		pAllocNode(bumpObject);
		
		// Append shaders to outNormal in shaders, after pull iteration.
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
			pAddNodeCallbacks(shaderObject);
			pAllocNode(shaderObject);
			
			pSendMaterialData(shaderObject);
		}
	}

	// NOTE: REMOVE OTHER ITERATORS AND JUST FETCH THE SHADER ENGINE, UNNECESSARY PULL PULSES

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
void pGetExistingScene()
{
	MString debugString {};
	MStatus res {};

	//Queries the current viewport.
	M3dView sceneView {sceneView.active3dView()};

	//Queries the active cameras DagPath.
	MDagPath camShapeDagPath {};
	sceneView.getCamera(camShapeDagPath);
	
	//Get root node
	MFnDagNode camDAG(camShapeDagPath.node());
	MFnDagNode rootDAG(camDAG.dagRoot());
	//MGlobal::displayInfo("ROOT successfully fetched!");

	MFnDependencyNode worldMatrixNode { rootDAG.object() };
	MObject worldObject {worldMatrixNode.object()};
	pAllocNode(worldObject);
	pSendTransformData(worldObject);
	//MGlobal::displayInfo("WorldMat Fetched!");

	pGetExistingMaterials();
	//MGlobal::displayInfo("Materials Fetched!");
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
				pAddNodeCallbacks(rootChildObject);
				pAllocNode(rootChildObject);
				pSendTransformData(rootChildObject);
				//MGlobal::displayInfo("reached children!");
				if (rootChildDAG.childCount())
				{
					pCreateAddCallbackChildNode(rootChildObject);
				}
			}
		}
	}
	
	pSendActiveCamera();
}

void preRenderCallback(const MString& str, void* clientData)
{
	//Check connection status
	//In-case dissconnect or reconnect -Clear buffer
	//Fetch existant objects
	MString debugString {};
	std::vector<char> msg {};
	size_t messageSize {};
	bool sending {};

	//M3dView sceneView{ sceneView.active3dView() };
	//sceneView.updateViewingParameters();

	//MMatrix viewMatrix {};
	//sceneView.modelViewMatrix(viewMatrix);
	//double det {viewMatrix.det4x4()};
	//MGlobal::displayInfo("############ modelViewMatrix: ");
	//debugString = "determinant: ";
	//debugString += det;
	//MGlobal::displayInfo(debugString);
	//pPrintMatrix(viewMatrix);
	
	//MGlobal::displayInfo("#############");

	if (comlib.connectionStatus->peekExistingMessage())
	{
		//MGlobal::displayInfo("GAY1");
		Connection_Status::CONNECTION_TYPE connectionType {};
		if (comlib.connectionStatus->checkConnection(connectionType) == S_OK)
		{
			switch (connectionType)
			{
			case Connection_Status::CONNECTION_TYPE::CONNECTED:
				//MGlobal::displayInfo("6.1");
				comlib.reset();
				comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::OFFSTART, messageSize);
				//#### Query all the existing data in the scene
				pGetExistingScene();
				//pGetParentChildRelationship();
				comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::OFFEND, messageSize);
				while (sending == false)
				{
					//MGlobal::displayInfo("Sending!");
					sending = comlib.send();
				}
				comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::ATTRST, messageSize);
				break;
			case Connection_Status::CONNECTION_TYPE::DISCONNECTED:
				comlib.mutex.Lock();
				//MGlobal::displayInfo("Reached a disconnection!");
				comlib.reset();
				comlib.mutex.Unlock();
				break;
			default:
				break;
			}
		}
	}
	else
	{
		//MGlobal::displayInfo("6.2");
		pSendActiveCamera();
		comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::ATTREND, messageSize);
		//MGlobal::displayInfo("6.3");
		while (sending == false)
		{
			//MGlobal::displayInfo("sending!");
			sending = comlib.send();
		}
		//MGlobal::displayInfo("6.4");
		comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::ATTRST, messageSize);
		//MGlobal::displayInfo("6.5");
	}
}

//void postRenderCallback(const MString& str, void* clientData)
//{
//	MGlobal::displayInfo("test");
//	//Check connection status
//	//In-case dissconnect or reconnect -Clear buffer
//	//Fetch existant objects
//	Connection_Status::CONNECTION_TYPE connectionType{};
//	if (SUCCEEDED(comlib.connectionStatus->checkConnection(connectionType)))
//	{
//		std::vector<char> msg{};
//		size_t messageSize{};
//		bool sending{};
//
//		M3dView sceneView{ sceneView.active3dView() };
//
//		MDagPath camShapeDagPath{};
//		sceneView.getCamera(camShapeDagPath);
//
//		MFnDagNode camDAG{ camShapeDagPath.node() };
//		MGlobal::displayInfo("6");
//		switch (connectionType)
//		{
//		case Connection_Status::CONNECTION_TYPE::CONNECTED:
//			MGlobal::displayInfo("6.1");
//			comlib.reset();
//			comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::OFFSTART, messageSize);
//			//#### Query all the existing data in the scene
//			pGetExistingScene();
//			//pGetParentChildRelationship();
//			comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::OFFEND, messageSize);
//			while (sending)
//			{
//				sending = comlib.send();
//			}
//			comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::ATTRST, messageSize);
//			break;
//		case Connection_Status::CONNECTION_TYPE::ALIVE:
//			MGlobal::displayInfo("6.2");
//			pSendActiveCamera(camDAG);
//			comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::ATTREND, messageSize);
//			MGlobal::displayInfo("6.3");
//			while (sending)
//			{
//				sending = comlib.send();
//			}
//			MGlobal::displayInfo("6.4");
//			comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::ATTRST, messageSize);
//			MGlobal::displayInfo("6.5");
//			break;
//		case Connection_Status::CONNECTION_TYPE::DISCONNECTED:
//			comlib.mutex.Lock();
//			MGlobal::displayInfo("Reached a disconnection!");
//			comlib.reset();
//			comlib.mutex.Unlock();
//			break;
//		default:
//			break;
//		}
//	}
//}

EXPORT MStatus initializePlugin(MObject obj)
{
	MStatus res{ MS::kFailure };
	MString debugString{};

	MFnPlugin myPlugin{ obj, "Maya plugin", "1.0", "Any", &res };
	if (MFAIL(res))
	{
		CHECK_MSTATUS(res);
		return res;
	}
	else
	{
		MGlobal::displayInfo("Maya plugin loaded!");
	}
	
		Connection_Status::CONNECTION_TYPE connectionType{};
		if (SUCCEEDED(comlib.connectionStatus->checkConnection(connectionType)))
		{
			std::vector<char> msg {};
			size_t messageSize {};
	
			//MGlobal::displayInfo("Switch:");
			switch (connectionType)
			{
			case Connection_Status::CONNECTION_TYPE::CONNECTED:
				comlib.reset();
				comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::OFFSTART, messageSize);
				//#### Query all the existing data in the scene
				pGetExistingScene();
				comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::OFFEND, messageSize);
				comlib.send();
				//MGlobal::displayInfo("Sending!");
				comlib.addToPackage(msg.data(), ComLib::MSG_TYPE::MESSAGE, ComLib::ATTRIBUTE_TYPE::ATTRST, messageSize);
				//MGlobal::displayInfo("Prepaired for attributes!");
				break;
			}
		}
		MCallbackId nodeAddedId{ MDGMessage::addNodeAddedCallback(
			pNodeCreationCallback,
			kDefaultNodeType,
			nullptr,
			&res) };
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(nodeAddedId) == MS::kSuccess) {};
		}
		
		MCallbackId nodeRemovedId{ MDGMessage::addNodeRemovedCallback(
			pNodeDeleteCallback,
			kDefaultNodeType,
			nullptr,
			&res) };
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(nodeRemovedId) == MS::kSuccess) {};
		}
		
		MCallbackId parentAddedID{ MDagMessage::addParentAddedCallback(
			pParentAddedCallback,
			nullptr,
			&res) };
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(parentAddedID) == MS::kSuccess) {};
		}
		
		MCallbackId parentRemovedID{ MDagMessage::addParentRemovedCallback(
			pParentRemovedCallback,
			nullptr,
			&res) };
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(parentAddedID) == MS::kSuccess) {};
		}
	
		//MCallbackId undoId = MEventMessage::addEventCallback(
		//	"Undo",
		//	undoCallback,
		//	NULL,
		//	&res);
		//if (res == MS::kSuccess)
		//{
		//	if (myCallbackArray.append(undoId) == MS::kSuccess) {};
		//}
		
		MCallbackId preRenderId1{ MUiMessage::add3dViewPreRenderMsgCallback(
			"modelPanel1",
			preRenderCallback,
			nullptr,
			&res
		)};
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(preRenderId1) == MS::kSuccess) {};
		}
		MCallbackId preRenderId2{ MUiMessage::add3dViewPreRenderMsgCallback(
		"modelPanel2",
		preRenderCallback,
		nullptr,
		&res
	) };
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(preRenderId2) == MS::kSuccess) {};
		}
		MCallbackId preRenderId3{ MUiMessage::add3dViewPreRenderMsgCallback(
		"modelPanel3",
		preRenderCallback,
		nullptr,
		&res
	) };
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(preRenderId3) == MS::kSuccess) {};
		}
		MCallbackId preRenderId4{ MUiMessage::add3dViewPreRenderMsgCallback(
		"modelPanel4",
		preRenderCallback,
		nullptr,
		&res
	) };
		if (res == MS::kSuccess)
		{
			if (myCallbackArray.append(preRenderId4) == MS::kSuccess) {};
		}

	
	//MCallbackId postRenderId{ MUiMessage::add3dViewPreRenderMsgCallback(
	//"modelPanel4",
	//postRenderCallback,
	//nullptr,
	//&res
	//)};
	//if (res == MS::kSuccess)
	//{
	//	if (myCallbackArray.append(postRenderId) == MS::kSuccess) {};
	//}

	return res;
}
EXPORT MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	//Removes callbacks from Maya! If plugin is unloaded, objects are still bound in maya?
	//Causing the application to crash. Take a deeper look how to properly remove callbacks.
	MMessage::removeCallbacks(myCallbackArray);
	MGlobal::displayInfo("Maya plugin unloaded!");

	return MS::kSuccess;
}