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
// #																										 #
// ###########################################################################################################

// Hindsight notes:
//	* Plugs data is obtainable through DataHandles/DataBlocks
//	* Is callbacks to transformers necessary? Data seem to always be available. Callbacks added to revieve smaller changes
//	* Parenting only happens to transformers. Two shapes can't share the same parent?
//
//  * Separate further! Parenting | micro data. Send smaller amount of data!
//  * Get Materials from HyperShader, not hard coded shaders (ex. kLambert) or all material.
//    Need to add callbacks to existing shader nodes.
//    Renderer is shader driven https://help.autodesk.com/view/MAYAUL/2019/ENU/?guid=__developer_Maya_SDK_MERGED_Viewport_2_0_API_Maya_Viewport_2_0_API_Guide_Rendering_Framework_Shader_Driven_Update_html
//    All materials will hang the application. Too many.
//    Check MFn::kAdskMaterial, ::kDiffuseMaterial, ::kMaterial (StingrayPBS 
//  * Can't find base color in StingrayPBR material in either attributes or plugs. (See how custom HardwareShaders work).
//  * Translators have manipulators/pivots that modify the transform matrix.  (World matrix -> transform matrix -> pivot matrix (objects real position)

// DOES NOT SUPPORT!!!
// DOES NOT SUPPORT!!!
// DOES NOT SUPPORT!!!
// DOES NOT SUPPORT!!!
// * REDO/UNDO
// * COMBINE
// * DELETE HISTORY

#include "maya_includes.h"
#include "ComLib.h"

MCallbackIdArray myCallbackArray;
ComLib comlib("sharedFileMap", 100000 * 1 << 20);

//Cam Micro Data
//void sendCamData(MObject& object, )
//{
//	MString debugString;
//	MStatus res;
//
//	MFnCamera cam(object);
//	MFn
//
//}

//General Micro Data
void pSendActiveCamera(MFnDagNode& camDAG)
{
	MString dagUuid = camDAG.uuid();
	MString dagName = camDAG.name();

	//comlib.send(&activeCam, ComLib::MSG::UPDATEVALUES, ComLib::MSG_TYPE::MAINCAM, sizeof(comlib::message::activeCam));
}
void pSendPlugData(MPlug& plug, MString ownerName, MString ownerUuid, std::string messageType)
{
	MStatus res;
	MString debugString;
	
	MFnAttribute attribute(plug);
	MString attributeName = attribute.name();
	MDataHandle dh;
		 
	if (plug.asMObject().apiType() == MFn::Type::kData3Float)
	{	
		plug.getValue(dh);
		MFloatVector fv = dh.asFloat3();
		MGlobal::displayInfo(attribute.name());
		MGlobal::displayInfo(plug.attribute().apiTypeStr());

		debugString = "x: ";
		debugString += fv.x;
		debugString += "x: ";
		debugString += fv.y;
		debugString += "x: ";
		debugString += fv.z;

		MGlobal::displayInfo(debugString);
		//comlib.send(&colorData, ComLib::MSG::UPDATEVALUES, ComLib::MSG_TYPE::COLOR, sizeof(colorData));
		//send color
	}
}

//Transform Micro Data
void pSendMatrixData(MObject& object)
{
	MString debugString;
	MStatus res;
	MFnTransform transform(object);
	MString transformUuid = transform.uuid();
	MString transformName = transform.name();

	//for (size_t i = 0; i < node.attributeCount(); i++)
	//{
	//	MFnAttribute attr(node.attribute(i));
	//	debugString = i;
	//	debugString += ": ";
	//	debugString += attr.name();
	//	MGlobal::displayInfo(debugString);
	//}

	MMatrix mat(transform.transformationMatrix());

	MGlobal::displayInfo("============================================");
	debugString = transformName;
	debugString += "\n\t";
	debugString += mat.matrix[0][0];
	debugString += " ";
	debugString += mat.matrix[1][0];
	debugString += " ";
	debugString += mat.matrix[2][0];
	debugString += " ";
	debugString += mat.matrix[3][0];
	debugString += "\n\t";

	debugString += mat.matrix[0][1];
	debugString += " ";
	debugString += mat.matrix[1][1];
	debugString += " ";
	debugString += mat.matrix[2][1];
	debugString += " ";
	debugString += mat.matrix[3][1];
	debugString += "\n\t";

	debugString += mat.matrix[0][2];
	debugString += " ";
	debugString += mat.matrix[1][2];
	debugString += " ";
	debugString += mat.matrix[2][2];
	debugString += " ";
	debugString += mat.matrix[3][2];
	debugString += "\n\t";

	debugString += mat.matrix[0][3];
	debugString += " ";
	debugString += mat.matrix[1][3];
	debugString += " ";
	debugString += mat.matrix[2][3];
	debugString += " ";
	debugString += mat.matrix[3][3];
	MGlobal::displayInfo(debugString);

	//comlib.send(&matrixData, ComLib::MSG::UPDATEVALUES, ComLib::MSG_TYPE::TRANSFORM, sizeof(comlib::message::matrixData));
}

//Camera Micro Data
void pSendProjectionMatrix(MObject& object)
{
	MStatus res;
	MString debugString;

	MFnCamera cam(object);
	MString camUuid = cam.uuid();
	MString camName = cam.name();

	MMatrix projectionMat = cam.projectionMatrix().matrix;
			
	MGlobal::displayInfo("(((((((((((((((((((((((((((((((((((((");
	debugString = camName;
	debugString += "\n\t";
	debugString += projectionMat.matrix[0][0];
	debugString += " ";
	debugString += projectionMat.matrix[1][0];
	debugString += " ";
	debugString += projectionMat.matrix[2][0];
	debugString += " ";
	debugString += projectionMat.matrix[3][0];
	debugString += "\n\t";

	debugString += projectionMat.matrix[0][1];
	debugString += " ";
	debugString += projectionMat.matrix[1][1];
	debugString += " ";
	debugString += projectionMat.matrix[2][1];
	debugString += " ";
	debugString += projectionMat.matrix[3][1];
	debugString += "\n\t";

	debugString += projectionMat.matrix[0][2];
	debugString += " ";
	debugString += projectionMat.matrix[1][2];
	debugString += " ";
	debugString += projectionMat.matrix[2][2];
	debugString += " ";
	debugString += projectionMat.matrix[3][2];
	debugString += "\n\t";

	debugString += projectionMat.matrix[0][3];
	debugString += " ";
	debugString += projectionMat.matrix[1][3];
	debugString += " ";
	debugString += projectionMat.matrix[2][3];
	debugString += " ";
	debugString += projectionMat.matrix[3][3];
	MGlobal::displayInfo(debugString);

	//comlib.send(&matrixData, ComLib::MSG::UPDATEVALUES, ComLib::MSG_TYPE::PROJECTION, sizeof(comlib::message::matrixData));
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
void pSendPointsData(MObject& object)
{
	MFnMesh mesh(object);
	MString meshUuid(mesh.uuid());
	MString meshName(mesh.name());

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

			//push_back into vecotr list
		}
		//Allocate new vertex list in dx11/12 if count has changed
		//comlib.send(&pointsData, ComLib::MSG::UPDATEVALUES, ComLib::MSG_TYPE::POINTS, sizeof(comlib::message::matrixData));
	}
}

//Material Micro Data
void pSendTextureData(MObject& object) 
{
	MFnDependencyNode texNode(object);
}

//Type Data
void pSendTransformData(MObject& object)
{	
	pSendMatrixData(object);
}
void pSendMeshData(MObject& object)
{
	
}
void pSendCameraData(MObject& object)
{
	pSendProjectionMatrix(object);
	pSendCamData(object);
}
void pSendLightData(MObject& object)
{
	
}
void pSendShaderGroupData(MObject& object)
{
	MStatus res;
	MString debugString;
	MFnDependencyNode shaderGroup(object);

	MFnAttribute attr(shaderGroup.attribute(27));
	MGlobal::displayInfo(attr.name());
	MGlobal::displayInfo(attr.object().apiTypeStr());

	MPlug surfaceShader(shaderGroup.findPlug("surfaceShader", res));
	if (surfaceShader.isConnected())
	{
		MGlobal::displayInfo("Connected!");
		MGlobal::displayInfo(surfaceShader.asMObject().apiTypeStr());

		MPlugArray shaders;
		surfaceShader.connectedTo(shaders, true, false);
		for (size_t i = 0; i < shaders.length(); ++i)
		{
			MFnDependencyNode shader(shaders[i].node());
			MGlobal::displayInfo(shader.name());
		}
	}
	else
	{
		MGlobal::displayInfo("not Connected!");
	}
}
void pSendMaterialData(MObject& object)
{
	MStatus res; 
	MString debugString;

	MGlobal::displayInfo("¤¤##¤¤###¤¤¤## MATERIAL DATA SENT");
	MFnDependencyNode materialNode(object);
	MString materialName = materialNode.name();
	MString materialUuid = materialNode.uuid();
	MGlobal::displayInfo("Material name: " + materialName);
	MGlobal::displayInfo("Material Uuid: " + materialUuid);
	debugString = "Type: ";
	debugString += object.apiTypeStr();
	MGlobal::displayInfo(debugString);
	if (materialNode.name().length() != 0)
	{
		MPlug colorPlug = materialNode.findPlug("color", res);
		if (colorPlug.asMObject().apiType() != MFn::kInvalid)
		{
			pSendPlugData(colorPlug, materialName, materialUuid, "color");
		}

		MPlug ambientColorPlug = materialNode.findPlug("ambientColor", res);
		if (ambientColorPlug.asMObject().apiType() != MFn::kInvalid)
		{
			pSendPlugData(ambientColorPlug, materialName, materialUuid, "ambientColor");
		}

		MPlug transparencyPlug = materialNode.findPlug("transparency", res);
		if (transparencyPlug.asMObject().apiType() != MFn::kInvalid)
		{
			pSendPlugData(transparencyPlug, materialName, materialUuid, "transparency");
		}
	}

	//MGlobal::displayInfo("_______________________");
	//for (size_t i = 0; i < materialNode.attributeCount(); i++)
	//{
	//	MFnAttribute attr(materialNode.attribute(i));
	//	MGlobal::displayInfo(attr.name());
	//}
	//MGlobal::displayInfo("_______________________");
}


void pAllocNode(MObject& object)
{
	//comlib::message::allocData{uuid, name, type};
	MFnDependencyNode node(object);
	MString nodeUuid(node.uuid());
	MString nodeName(node.name());

	if (nodeName.length() == 0)
	{
		MFnDagNode dag(object);
		MString dagUuid(dag.uuid());
		MString dagName(dag.name());

		//MGlobal::displayInfo("¤/¤/¤/¤/¤/¤/");
		//MGlobal::displayInfo("dagNode:");
		//MGlobal::displayInfo(dagName);
		//MGlobal::displayInfo(dagUuid);

		//Switch to enum?
		//MString dagType(dag.typeName());
		//MGlobal::displayInfo(dagType);
		//MGlobal::displayInfo("¤/¤/¤/¤/¤/¤/");
	}
	else
	{
		//MGlobal::displayInfo("¤/¤/¤/¤/¤/¤/");
		//MGlobal::displayInfo("dgNode:");
		//MGlobal::displayInfo(nodeUuid);
		//MGlobal::displayInfo(nodeName);
		//
		////Switch to enum?
		//MString nodeType(node.typeName());
		//MGlobal::displayInfo(nodeType);
		//MGlobal::displayInfo("¤/¤/¤/¤/¤/¤/");

		//comlib.send(&allocData, ComLib::MSG::ALLOCATE, sizeof(comlib::message::allocData));
	}
}
void pSendParentsData(MObject& object)
{
	MFnDagNode dag(object);
	MString dagUuid = dag.uuid();
	MString dagName = dag.name();

	//MGlobal::displayInfo("##Parent Data sent");

	for (UINT i = 0; i < dag.parentCount(); i++)
	{
		MFnDependencyNode parentNode(dag.parent(i));
		//Comlib::vector = parentNode.uuid();
		//ComLib::vector = parentNode.name();
	}

	//comlib.send(&parentData, ComLib::MSG::UPDATEVALUES, ComLib::MSG_TYPE::PARENTS, sizeof(comlib::message::parentData));
}

void pNameChangeCallback(MObject& node, const MString& lastName, void* clientData)
{

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
		debugString = "CHANGED ATTRIBUTE NAME: ";
		debugString += attributeName;
		MGlobal::displayInfo(debugString);
		MGlobal::displayInfo(plug.node().apiTypeStr());

		//MGlobal::displayInfo("Attribute changed");
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
				MGlobal::displayInfo(node.name());
				pSendMatrixData(nodeObject);
			}
			break;
		case MFn::kCamera:
			MGlobal::displayInfo(node.name());
			MGlobal::displayInfo(dag.name());
			MGlobal::displayInfo(attributeName);

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
			if (attributeName == "pnts" ||
				attributeName == "uvPivot")
			{
				pSendPointsData(nodeObject);
			}

			break;
		case MFn::kShadingEngine:
			if (attributeName == "surfaceShader")
			{

			}
			break;
		//case MFn::kMaterial:
		//	MGlobal::displayInfo("material info changed");
		//	break;
		case MFn::kLambert:
			if (attributeName == "color" ||
				attributeName == "transparency" ||
				attributeName == "ambientColor")
			{
			}
			else if (attributeName == "normalCamera")
			{
				// Get normal texture
			   // Connected bump2d1.outNormal 
			}
			break;
		case MFn::kFileTexture:
			MGlobal::displayInfo("get texture path");
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

		if (ownerType == MFn::kMesh)
		{

		}
		else if (ownerType == MFn::kLambert &&
				 ownerType == MFn::kBlinn)
		{
			if (attributeName == "color")
			{
				//  Connected file3.outColor -> blinn.color

				// send texture data
			}
			else if (attributeName == "normalCamera")
			{
				 // Get normal texture
				// Connected Blinn.normalCamera -> bump2d.outNormal -> Bump2d.bumpValue -> file.outAlpha
				// send normal tex data
			}
		}
		if (ownerType == MFn::kFileTexture)
		{
			if (attributeName == "outColor")
			{

			}
		}

		//if (ownerType == MFn::kShadingEngine)
		//{
		//	//MGlobal::displayInfo("(/&/&/&%(&%&%");
		//	//MPlug plug = node.findPlug("surfaceShader", res);
		//	//MPlugArray cPlugArray;
		//	//plug.connectedTo(cPlugArray, true, false);
		//	//for (size_t i = 0; i < cPlugArray.length(); ++i)
		//	//{
		//	//	MPlug materialPlug(cPlugArray[i]);
		//	//	MFnDependencyNode materialNode(materialPlug.node());
		//	//	MGlobal::displayInfo(materialNode.name());
		//	//	MGlobal::displayInfo(materialNode.typeName());


		//	//}
		//	//MGlobal::displayInfo("(/&/&/&%(&%&%");
		//}
	}
}

void pParentAddedCallback(MDagPath& child, MDagPath& parent, void* clientData)
{
	MFnDagNode dag(child);
	MString objectUuid(dag.uuid());
	MString objectName(dag.name());

	if (objectName.length() != 0)
	{
		//MGlobal::displayInfo("***************");
		//MGlobal::displayInfo(objectName);
		//MGlobal::displayInfo("***************");


		//MFnDagNode parentDAG(parent);
		//MString parentUuid(parentDAG.uuid());
		//MString parentName(parentDAG.name());

		//MGlobal::displayInfo("***************");
		//MGlobal::displayInfo(parentName);
		//MGlobal::displayInfo("***************");

		//MGlobal::displayInfo("##Entered childhood");
	}
	//comlib.send(&parentData, ComLib::MSG::NEWVALUES, ComLib::MSG_TYPE::PARENTS, sizeof(comlib::message::parentData));
}
void pParentRemovedCallback(MDagPath& child, MDagPath& parent, void* clientData)
{
	MFnDagNode dag(child);
	MString objectUuid(dag.uuid());
	MString objectName(dag.name());

	if (objectName.length() != 0)
	{
		MFnDagNode parentDAG(parent);
		MString parentUuid(parentDAG.uuid());
		MString parentName(parentDAG.name());
	
		//comlib.send(&parentData, ComLib::MSG::REMOVEVALUES, ComLib::MSG_TYPE::PARENTS, sizeof(comlib::message::parentData));
	}
}

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

	MCallbackId AttributeChangedID = MNodeMessage::addAttributeChangedCallback(
		object,
		pAttributeChangedCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(AttributeChangedID) == MS::kSuccess) {};
	}
}
void pCreateAddCallbackChildNode(MObject currentObject)
{
	MString debugString;
	MStatus res;

	MFnDagNode currentDag(currentObject);

	for (UINT i = 0; i < currentDag.childCount(); ++i)
	{
		MObject childObject(currentDag.child(i));
		MFnDagNode childDAG(childObject);

		//MGlobal::displayInfo(childDAG.typeName());

		switch (childObject.apiType())
		{
		case MFn::kTransform:
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendParentsData(childObject);
			pSendTransformData(childObject);
			break;
		
		case MFn::kMesh:
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendParentsData(childObject);
			break;

		case MFn::kCamera:
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendParentsData(childObject);
			pSendCameraData(childObject);
			break;

		case MFn::kLight:
			pAddNodeCallbacks(childObject);
			pAllocNode(childObject);
			pSendParentsData(childObject);
			pSendLightData(childObject);
			break;
		//case MFn::kShadingEngine:
		//	pAddNodeCallbacks(childObject);
		//	pAllocNode(childObject);
		//	pSendParentsData(childObject);
		//	//pSendShadingGroupData(childObject);

		default:
			break;
		}

		if (childDAG.childCount() > 0)
		{
			pCreateAddCallbackChildNode(childObject);
		}
	}

}

void pNodeCreationCallback(MObject& object, void* clientData)
{
	MStatus res;
	MString debugString;
	MGlobal::displayInfo("##NodeCreated");
	MFnDependencyNode node(object);
	MGlobal::displayInfo(object.apiTypeStr());
	debugString = object.apiType();
	MGlobal::displayInfo(debugString);
	switch (object.apiType())
	{
	case MFn::kTransform:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendParentsData(object);
		//pSendTransformData(object);
	case MFn::kMesh:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendParentsData(object);
		break;
	case MFn::kCamera:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendParentsData(object);
		pSendCameraData(object);
		break;
	case MFn::kLight:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendParentsData(object);
		break;
	case MFn::kShadingEngine:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendShaderGroupData(object);
		break;

	// Surface Materials
	case MFn::kLambert:
	case MFn::kBlinn:
	case MFn::kPhong:
	case MFn::kPhongExplorer:
	case MFn::kLayeredShader:
	case MFn::kRampShader:
	case MFn::kShadingMap:
	case MFn::kHairTubeShader:
		pAddNodeCallbacks(object);
		pAllocNode(object);
		pSendMaterialData(object);
		break;
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
	MFnDagNode currentDag(object);
	if (currentDag.childCount() > 0)
	{
		pCreateAddCallbackChildNode(object);
	}
}
void pNodeDeleteCallback(MObject& object, void* clientData)
{
	
}

void pGetExistingMaterials()
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

	//Iterate through all materials (Lambert, Blinn, Phong, DirectX etc.)
	MGlobal::displayInfo("*************************************");
	MGlobal::displayInfo("Shaders: ");
	for (size_t i = 0; i < type.size(); ++i)
	{
		MItDependencyNodes nodeIt(type.at(i));
		for (; !nodeIt.isDone(); nodeIt.next())
		{
			MObject shaderObject(nodeIt.thisNode());
			pAddNodeCallbacks(shaderObject);
			pAllocNode(shaderObject);
			
			pSendMaterialData(shaderObject);
		}
	}
	MGlobal::displayInfo("*************************************");

	MItDependencyNodes nodeIt(MFn::kShadingEngine);
	for (; !nodeIt.isDone(); nodeIt.next())
	{
		MFnDependencyNode node(nodeIt.thisNode());
		MObject groupObject(nodeIt.thisNode());
		if (node.name() != "initialParticleSE")
		{
			MGlobal::displayInfo(node.name());
			MGlobal::displayInfo(node.typeName());
			pAddNodeCallbacks(groupObject);
			pAllocNode(groupObject);

			pSendShaderGroupData(groupObject);
		}
	}

}
void getExistingScene()
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
			//MGlobal::displayInfo(rootChildName);

			//Hard-coded. Possible automation
			//Essencial scene objects, carefull when modifying
			if (rootChildName != "groundPlane_transform"
				&& rootChildName != "defaultUfeProxyParent"
				&& rootChildName != "shaderBallCamera1"
				&& rootChildName != "shaderBallOrthoCamera1"
				&& rootChildName != "MayaMtlView_FillLight1"
				&& rootChildName != "MayaMtlView_RimLight1")
			{
				pAddNodeCallbacks(rootChildObject);
				
				pAllocNode(rootChildObject);
				
				pSendParentsData(rootChildObject);
				pSendTransformData(rootChildObject);

				if (rootChildDAG.childCount())
				{
					pCreateAddCallbackChildNode(rootChildObject);
				}
			}
		}
	}

	pGetExistingMaterials();

	pSendActiveCamera(camDAG);
}

void renderChangeCallback(const MString& str, void* clientData)
{
	M3dView sceneView;
	sceneView = sceneView.active3dView();

	MDagPath camShapeDagPath;
	sceneView.getCamera(camShapeDagPath);

	MFnDagNode camDAG(camShapeDagPath.node());

	pSendActiveCamera(camDAG);
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
		pNodeCreationCallback,
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

	MCallbackId parentAddedID = MDagMessage::addParentAddedCallback(
		pParentAddedCallback,
		NULL,
		&res);
	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(parentAddedID) == MS::kSuccess) {};
	}

	MCallbackId parentRemovedID = MDagMessage::addParentRemovedCallback(
		pParentRemovedCallback,
		NULL,
		&res);
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
	//

	MCallbackId camTranslateId = MUiMessage::add3dViewPostRenderMsgCallback(
		"modelPanel4",
		renderChangeCallback,
		NULL,
		&res
	);

	return res;
}
EXPORT MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	MMessage::removeCallbacks(myCallbackArray);
	MGlobal::displayInfo("Maya plugin unloaded!");

	return MS::kSuccess;
}