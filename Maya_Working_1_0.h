// ############################################################################################### 
// 
// 			Messy but working version of the code. Contains notes and alternative methods.
// 
// ###############################################################################################

#include "maya_includes.h"
#include "ComLib.h"
#include "Plugin_Storage.h"

MCallbackIdArray myCallbackArray;
ComLib comlib("sharedFileMap", 100000 * 1 << 20);

//When the in-game scene camera changes
void renderChangeCallback(const MString &str, void *clientData)
{
	//Fetches the current viewport.
	M3dView sceneView;
	sceneView = sceneView.active3dView();
	//Fetches the current camera.
	MDagPath camDag;
	sceneView.getCamera(camDag);

	//Checks if it actually is a camera.
	if (camDag.node().apiType() == MFn::kCamera)
	{
		MFnCamera cam = camDag.node();
		//Fetches the parent of the camera which will always be a transformer
		MFnTransform camTransform = cam.parent(0);
		MMatrix viewMat = camTransform.transformation().asMatrix().matrix;
		MFloatMatrix projectionMat = cam.projectionMatrix();

		MString camName = camTransform.absoluteName();
		//MGlobal::displayInfo(MString("CameraName: " + camName));
		//
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[0][0] + " " + viewMat.matrix[0][1] + " " + viewMat.matrix[0][2] + " " + viewMat.matrix[0][3]);
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[1][0] + " " + viewMat.matrix[1][1] + " " + viewMat.matrix[1][2] + " " + viewMat.matrix[1][3]);
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[2][0] + " " + viewMat.matrix[2][1] + " " + viewMat.matrix[2][2] + " " + viewMat.matrix[2][3]);
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[3][0] + " " + viewMat.matrix[3][1] + " " + viewMat.matrix[3][2] + " " + viewMat.matrix[3][3]);
		//
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[0][0] + " " + projectionMat.matrix[0][1] + " " + projectionMat.matrix[0][2] + " " + projectionMat.matrix[0][3]);
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[1][0] + " " + projectionMat.matrix[1][1] + " " + projectionMat.matrix[1][2] + " " + projectionMat.matrix[1][3]);
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[2][0] + " " + projectionMat.matrix[2][1] + " " + projectionMat.matrix[2][2] + " " + projectionMat.matrix[2][3]);
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[3][0] + " " + projectionMat.matrix[3][1] + " " + projectionMat.matrix[3][2] + " " + projectionMat.matrix[3][3]);

		double ARO = cam.aspectRatio();
		double HFOV = cam.horizontalFieldOfView();
		double VFOV = cam.verticalFieldOfView();
		double nPlane = cam.nearClippingPlane();
		double fPlane = cam.farClippingPlane();

		//MGlobal::displayInfo(MString("Cam:") + ARO);
		//MGlobal::displayInfo(MString("Cam:") + HFOV);
		//MGlobal::displayInfo(MString("Cam:") + VFOV);
		//MGlobal::displayInfo(MString("Cam:") + nPlane);
		//MGlobal::displayInfo(MString("Cam:") + fPlane);

		ComLib::CameraData camData;

		//ViewMat
		camData.viewMat[0][0] = viewMat.matrix[0][0];
		camData.viewMat[1][0] = viewMat.matrix[1][0];
		camData.viewMat[2][0] = viewMat.matrix[2][0];
		camData.viewMat[3][0] = viewMat.matrix[3][0];

		camData.viewMat[0][1] = viewMat.matrix[0][1];
		camData.viewMat[1][1] = viewMat.matrix[1][1];
		camData.viewMat[2][1] = viewMat.matrix[2][1];
		camData.viewMat[3][1] = viewMat.matrix[3][1];

		camData.viewMat[0][2] = viewMat.matrix[0][2];
		camData.viewMat[1][2] = viewMat.matrix[1][2];
		camData.viewMat[2][2] = viewMat.matrix[2][2];
		camData.viewMat[3][2] = viewMat.matrix[3][2];

		camData.viewMat[0][3] = viewMat.matrix[0][3];
		camData.viewMat[1][3] = viewMat.matrix[1][3];
		camData.viewMat[2][3] = viewMat.matrix[2][3];
		camData.viewMat[3][3] = viewMat.matrix[3][3];

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

		//comlib.send(&camData, ComLib::MSG_TYPE::CAMERA, sizeof(ComLib::CameraData));
	}

}

void getTransfromData(MFnTransform& transformer)
{
	MString dispScale;
	MString dispTrans;
	MString dispQuat;

	MMatrix mat = transformer.transformation().asMatrix().matrix;

	//MString transformerName = transformer.absoluteName();
	MFnDependencyNode transformerNode(transformer.object());
	MString transformerName = transformerNode.name();
	MGlobal::displayInfo(MString("Name: " + transformerName));
	MGlobal::displayInfo(MString("Transformer:") + mat.matrix[0][0] + " " + mat.matrix[0][1] + " " + mat.matrix[0][2] + " " + mat.matrix[0][3]);
	MGlobal::displayInfo(MString("Transformer:") + mat.matrix[1][0] + " " + mat.matrix[1][1] + " " + mat.matrix[1][2] + " " + mat.matrix[1][3]);
	MGlobal::displayInfo(MString("Transformer:") + mat.matrix[2][0] + " " + mat.matrix[2][1] + " " + mat.matrix[2][2] + " " + mat.matrix[2][3]);
	MGlobal::displayInfo(MString("Transformer:") + mat.matrix[3][0] + " " + mat.matrix[3][1] + " " + mat.matrix[3][2] + " " + mat.matrix[3][3]);

	ComLib::TransformerData transformData;
	transformData.transformerName = transformerName.asChar();
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
	transformData.parentName.clear();

	MString debug = "Parent Count: ";
	debug += parentCount;
	MGlobal::displayInfo(debug);
	for (int i = 0; i < parentCount; i++)
	{
		MObject parentObject(transformer.parent(i));

		MGlobal::displayInfo(MString("Parent type: ") + parentObject.apiTypeStr());
		
		if (parentObject.apiType() == MFn::kWorld)
		{
			transformData.parentName.push_back("None");
		}
		else if (parentObject.apiType() == MFn::kMesh)
		{
			MFnMesh parent(parentObject);
			transformData.parentName.push_back(transformer.name().asChar());
			//MGlobal::displayInfo(transformer.name().asChar());
		}
		else if (parentObject.apiType() == MFn::kTransform)
		{
			MFnTransform parent(parentObject);
			transformData.parentName.push_back(transformer.name().asChar());
			//MGlobal::displayInfo(transformer.name().asChar());
		}
	}

	//comlib.send(&transformData, ComLib::MSG_TYPE::TRANSFORMER, sizeof(ComLib::TransformerData));

	/*
	//Scale
	double scale[3] = { 1, 1, 1 };
	transformer.getScale(scale);

	dispScale += "Scale: ";
	dispScale += "X: ";
	dispScale += scale[0];
	dispScale += "Y: ";
	dispScale += scale[1];
	dispScale += "Z: ";
	dispScale += scale[2];

	//Translation
	MVector translation = transformer.getTranslation(MSpace::kObject);

	dispTrans += "Translation: ";
	dispTrans += "X: ";
	dispTrans += translation.x;
	dispTrans += "Y: ";
	dispTrans += translation.y;
	dispTrans += "Z: ";
	dispTrans += translation.z;

	//Rotation
	MQuaternion quaternion(0, 0, 0, 1);
	transformer.getRotation(quaternion);
	dispQuat += "Quaternion rotation: ";
	dispQuat += "X: ";
	dispQuat += quaternion.x;
	dispQuat += "Y: ";
	dispQuat += quaternion.y;
	dispQuat += "Z: ";
	dispQuat += quaternion.z;
	dispQuat += "W: ";
	dispQuat += quaternion.w;

	MGlobal::displayInfo(dispScale);
	MGlobal::displayInfo(dispTrans);
	MGlobal::displayInfo(dispQuat);
	*/
}

void outputMeshData(MFnMesh& mesh)
{
	ComLib::MeshData meshData;
	MString meshName;
	MString displayLength;
	MString displayCoords = "\n";

	ComLib::MeshData::Vertex vertex;
	ComLib::MeshData::Normals normal;
	ComLib::MeshData::UV uv;

	meshName = mesh.name();
	//MGlobal::displayInfo(meshName);
	meshData.meshName = meshName.asChar();

	// Vertex count per polygon ("face") AFFECTED BY TRIANGULATE! | vertexList ("vertex ID") Global(mesh-relative/object-relative)
	MIntArray vertexCount;
	MIntArray vertexList;
	mesh.getVertices(vertexCount, vertexList);

	MString debug;

	//MGlobal::displayInfo("######## vertex Count ###########");
	//for (UINT i = 0; i < vertexCount.length(); i++)
	//{
	//	debug = vertexCount[i];
	//	MGlobal::displayInfo(debug);
	//}
	//MGlobal::displayInfo("######### vertexList ##########");
	//for (UINT i = 0; i < vertexList.length(); i++)
	//{
	//	debug = vertexList[i];
	//	MGlobal::displayInfo(debug);
	//}

	//Verticis
	MPointArray vts;
	mesh.getPoints(vts,MSpace::kObject);
	float test[4];

	/*MGlobal::displayInfo("####### Point List ######");

	for (UINT i = 0; i < vts.length(); i++)
	{
		vts[i].get(test);
		debug = "x = ";
		debug += test[0];
		debug += " | y = ";
		debug += test[1];
		debug += " | z = ";
		debug += test[3];
		MGlobal::displayInfo(debug);
	}*/

	displayLength = "Number of vertisis: ";
	displayLength += vts.length();
	//MGlobal::displayInfo(displayLength);

	if (vts.length() <= 0)
	{
		return;
	}

	for (int i = 0; i != vts.length(); ++i)
	{
		ComLib::MeshData::Vertex vertex;

		displayCoords += "X: ";
		displayCoords += vts[i].x;
		vertex.VertexX = vts[i].x;
		displayCoords += " Y: ";
		displayCoords += vts[i].y;
		vertex.VertexY = vts[i].y;
		displayCoords += " Z: ";
		displayCoords += vts[i].z;
		vertex.VertexZ = vts[i].z;
		displayCoords += "\n";
	}
	//MGlobal::displayInfo(displayCoords);

	//Normals
	MFloatVectorArray nmls;

	mesh.getNormals(nmls);
	//MIntArray normalCount;
	//MIntArray normalIDs;
	//mesh.getNormals(normals);
	//mesh.getVertexNormals(true, normals, MSpace::kObject);
	//mesh.getNormalIds(normalCount, normalIDs);
	
	displayLength = "Number of normals: ";
	displayLength += nmls.length();
	//MGlobal::displayInfo(displayLength);

	displayCoords = "\n";

	for (int i = 0; i != nmls.length(); ++i)
	{
		displayCoords += "X: ";
		displayCoords += nmls[i].x;
		normal.NormalX = nmls[i].x;
		displayCoords += " Y: ";
		displayCoords += nmls[i].y;
		normal.NormalY = nmls[i].y;
		displayCoords += " Z: ";
		displayCoords += nmls[i].z;
		normal.NormalZ = nmls[i].z;
		displayCoords += "\n";

		meshData.normalsList.push_back(normal);
	}
	//MGlobal::displayInfo(displayCoords);

	//// Triangulates given vertex positions not IDs. 8 vertex (cube) gives 18 IDs, not 36. 
	//// Triangulates logically not the entire mesh (cube).
	//status = mesh.polyTriangulate(vertexArray, holesList, vertexArray.length(), normals, vertexList);
	//if (status == MS::kSuccess)
	//{
	//	MGlobal::displayInfo("major success");
	//	mesh.getNormals(normals, MSpace::kObject);
	//	mesh.getNormalIds(normalCount, normalList);
	//}
	//else
	//{
	//	MGlobal::displayInfo("major fail");
	//	MGlobal::displayInfo(status.errorString());
	//}



	//Texture Coordinates
	bool existingUV = true;
	//uvSets used when multi-texturing (decals) | add support later
	MStringArray uvSets;
	mesh.getUVSetNames(uvSets);

	if (!uvSets.length() || !mesh.numUVs(uvSets[0]))
	{
		//MGlobal::displayInfo("no uvs");
	}
	else
	{
			////### v1.1 ##########################
			//for (UINT i = 0; i < uvSets.length(); ++i)
			//{
			//	mesh.getUVs(UCoords, VCoords, &uvSets[i]);
			//	mesh.getAssignedUVs(UVCounts, UVList, &uvSets[i]);
			//	UVs.clear();
			//	for (UINT j = 0; j < UCoords.length(); ++j)
			//	{
			//		uv.U = UCoords[j];
			//		uv.V = VCoords[j];
			//		UVs.push_back(uv);
			//	}
			//	meshData.uvLists.push_back(UVs);
			//	//Add uvs to meshData
			//}
			////##################################
		
		displayLength = ("length of UV coordinates: ");
		displayLength += uvSets.length();
		//MGlobal::displayInfo(displayLength);

		displayCoords = "";
		for (int i = 0; i != uvSets.length(); ++i)
		{
			//to see the name
			displayCoords += ("Uv Name: ");
			displayCoords += uvSets[i] + " \n";

			//MObjectArray textureArray;
			//mesh.getAssociatedUVSetTextures(uvSets[i], textureArray);
			//
			//debug = "Length ";
			//debug += textureArray.length();
			//MGlobal::displayInfo(debug);

			//for (size_t i = 0; i < textureArray.length(); i++)
			//{
			//	//MString test;
			//	//MObject tex;
			//	//tex = textureArray[i];
			//	//MTexture tex1(tex);

			//	//test += textureArray[i];
			//	//MGlobal::displayInfo("##############");
			//	//MGlobal::displayInfo(test);
			//}

			MFloatArray UCoords;
			MFloatArray VCoords;

			mesh.getUVs(UCoords, VCoords, &uvSets[i]);

			displayCoords += "number of UV: ";
			displayCoords += mesh.numUVs(uvSets[i]);
			displayCoords += "\n";

			for (int j = 0; j != mesh.numUVs(uvSets[i]); ++j)
			{
				int numUVs = mesh.numUVs(uvSets[j]);

				uv.U = UCoords[j];
				uv.V = VCoords[j];
				meshData.uvList.push_back(uv);

				displayCoords += UCoords[j];
				displayCoords += " ";
				displayCoords += VCoords[j];
				displayCoords += "\n";

			}

			//Connected Texture Nodes
			//MString texturePath;
			//MPlug fullPath = mesh.findPlug("ftn"); //File Texture Name = ftn
			//fullPath.getValue(texturePath);

			//MGlobal::displayInfo(texturePath);
			//meshData.texturePath = texturePath.asChar();

		}

		//MGlobal::displayInfo(displayCoords);
	}

	MObjectArray shaderArray;
	MIntArray faceIndiciesArray;
	mesh.getConnectedShaders(0, shaderArray, faceIndiciesArray);
	for (size_t i = 0; i < shaderArray.length(); i++)
	{
		//MGlobal::displayInfo(shaderArray[i].apiTypeStr());

		MPlugArray connections;
		MStatus status;
		MFnDependencyNode shaderGroup(shaderArray[i]);
		//MGlobal::displayInfo(shaderGroup.absoluteName().asChar());
		
		////Get PlugNames | Get Attribute names is a better choise!
		//MPlugArray plugArr;
		//shaderGroup.getConnections(plugArr);
		//for (size_t j = 0; j < plugArr.length(); j++)
		//{
		//	MPlug plug = plugArr[j];
		//	MGlobal::displayInfo(plug.info());
		//}
		
		////Get destination PlugNames | Get Attribute names is a better choise!
		//MPlugArray materialPlugArr;
		//materialNode.getConnections(materialPlugArr);
		//for (size_t k = 0; k < materialPlugArr.length(); k++)
		//{
		//	// Still .outColor??????? Why?
		//	MPlug plug = materialPlugArr[k];

		//	MPlugArray dst;
		//	materialPlugArr[k].destinations(dst);
		//	for (UINT l = 0; l < dst.length(); l++)
		//	{
		//		MGlobal::displayInfo(dst[l].info());
		//	}
		//}

		//// Fetches Attribute Names!
		//UINT attrCount = materialNode.attributeCount();
		//for (UINT k = 0; k < attrCount; k++)
		//{
		//	MFnAttribute attr(materialNode.attribute(k));
		//	debugString = "attr: ";
		//	debugString += attr.name();
		//	MGlobal::displayInfo(debugString);
		//}

		MPlug shaderPlug = shaderGroup.findPlug("surfaceShader",status);

		shaderPlug.connectedTo(connections, true, false);
		for (uint u = 0; u < connections.length(); u++)
		{
			MPlug plug = connections[u];
			//MGlobal::displayInfo(plug.info());

			//MGlobal::displayInfo(connections[u].name().asChar());

			//MString colorV;
			//MColor color(connections[u].asFloat());
			//colorV = "R: ";
			//colorV += color.r;
			//colorV += " G: ";
			//colorV += color.g; 
			//colorV += " B: ";
			//colorV += color.b;
			//MGlobal::displayInfo(colorV);

			if (connections[u].node().hasFn(MFn::kLambert))
			{
				MPlugArray plugs;
				MFnLambertShader lambertShader(connections[u].node());
				lambertShader.getConnections(plugs);

				//lambertShader.findPlug("fileTexture",status).connectedTo(plugs, true, false);
				for (uint y = 0; y < plugs.length(); y++)
				{
					MPlug lamPlug = plugs[y];
					//MGlobal::displayInfo(plug.info());
					//MGlobal::displayInfo(plugs[y].name().asChar());
					MFnDependencyNode matNode(plugs[i].node());

					MPlug colorPlug = matNode.findPlug("color", status);
					//MGlobal::displayInfo(colorPlug.info());
					//MGlobal::displayInfo(status.errorString());

					MItDependencyGraph dgIt(
						colorPlug,
						MFn::kFileTexture,
						MItDependencyGraph::kUpstream,
						MItDependencyGraph::kBreadthFirst,
						MItDependencyGraph::kNodeLevel,
						&status
					);
					dgIt.disablePruningOnFilter();

					if (!dgIt.isDone())
					{
						MObject textureNode = dgIt.currentItem();
						MPlug fileNamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName", status);
						MString texturePath;
						fileNamePlug.getValue(texturePath);
						//MGlobal::displayInfo(texturePath.asChar());

					}







					/*MPlugArray colorPlugs;
					colorPlug.connectedTo(colorPlugs, true, false);

					for (size_t j = 0; j < colorPlugs.length(); j++)
					{
						MPlug texture = colorPlugs[j];
						MGlobal::displayInfo(texture.info());
						MFnDependencyNode texPathNode(texture.node());
						MGlobal::displayInfo(texPathNode.absoluteName());
						MGlobal::displayInfo(texPathNode.typeName());

						MPlug filePath = texPathNode.findPlug("fileTextureName", status);
						MString str;
						filePath.getValue(str);

					}*/
					//MPlugArray imagePlug;
					//imageNode.findPlug("Image Name", status).connectedTo(imagePlug, true, false);
				}
			}
		}
	}
	//previous function used the bool as return value? Perhaps to not load texture if there was no UV coordinates?

	MStatus res;

	//Might need to calculate vector<string> * item count if project crashes here
	comlib.send(&meshData, ComLib::MSG_TYPE::MESH, sizeof(ComLib::MeshData));
}

void nameChangeCallback(MObject& node, const MString &lastName, void* clientData)
{
	// Fetches the name of the object that has been changed.
	if (node.hasFn(MFn::kMesh))
	{
		MFnMesh mesh(node);
		MString msg(mesh.name());
		//MGlobal::displayInfo("-> Rename: " + msg);
		//MGlobal::displayInfo("->LastName: " + lastName);


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

void attributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void*clientData)
{
	MStatus res;
	MObject obj(plug.node());

	if (msg & MNodeMessage::AttributeMessage::kAttributeSet)
	{
		if (obj.apiType() == MFn::kMesh)
		{
			MFnMesh mesh(obj);
			//MGlobal::displayInfo("Mesh Attribute Changed");
			outputMeshData(mesh);
			//TexturePath(mesh);
		}
		else if (obj.apiType() == MFn::kTransform)
		{
			MFnTransform transformer(obj);
			//MGlobal::displayInfo("Transfrom Attribute Changed");
			getTransfromData(transformer);
		}

		////Test for simplified shaders
		//if (obj.apiType() == MFn::kBlinn)
		//{
		//	MGlobal::displayInfo("Blinn Material");
		//	MFnBlinnShader ColorValues(obj);
		//	MGlobal::displayInfo(ColorValues.name());
		//}

		//plug.node() same as MObject?
		if (plug.node().hasFn(MFn::kBlinn))
		{
			//MGlobal::displayInfo("Blinn Material");
			//MGlobal::displayInfo(plug.name());
			MFnBlinnShader ColorValues = plug.node();
			MColor color;
			color = ColorValues.color();
			MString displayColor;
			displayColor += color.r;
			displayColor += " ";
			displayColor += color.g;
			displayColor += " ";
			displayColor += color.b;

			MGlobal::displayInfo(displayColor);

		}
		else if (plug.node().hasFn(MFn::kLambert))
		{
			//MGlobal::displayInfo("Lambert Material");
			//MGlobal::displayInfo(plug.name());
			MFnLambertShader ColorValues = plug.node();
			MColor color;
			color = ColorValues.color();
			MString displayColor;

			displayColor += color.r;
			displayColor += " ";
			displayColor += color.g;
			displayColor += " ";
			displayColor += color.b;

			MGlobal::displayInfo(displayColor);

		}
	}
}

void nodeCreationCallback(MObject& object, void* clientData)
{
	//Creates an empty node that later get's filled with information.

	//When first creating objects in the scene, callbacks get's executed even if the node has no
	//information. It's just an empty node with a label (ex. mesh). Can this be fixed?
	//if (object.apiType() == MFn::kMesh)
	//{
	//	MFnMesh mesh(object);
	//	outputMeshData(mesh);
	//}
	// Possibly that the mesh is created along with the transform. Since transformer has compute() and contains information.
	// But mesh gets its values afterwards in attributeChangedCallback. Is this because of callbacks? Why in this order?
	//MString debug;
	//debug = object.apiTypeStr();
	//MGlobal::displayInfo(debug);

	MStatus res;

	else if (object.apiType() == MFn::kTransform)
	{
		MFnTransform trans(object);
		getTransfromData(trans);
	}


	MCallbackId meshNameChangeID = MNodeMessage::addNameChangedCallback(
		object,
		nameChangeCallback,
		&clientData,
		&res);

	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(meshNameChangeID) == MS::kSuccess) {};
	}

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

	//MStringArray eventNames;
	//MEventMessage::getEventNames(eventNames);
	//for (uint i = 0; i < eventNames.length(); i++)
	//{
	//	MGlobal::displayInfo(eventNames[i]);
	//}

	M3dView sceneView;
	sceneView = sceneView.active3dView();
	//Fetches the current camera.
	MDagPath camDag;
	sceneView.getCamera(camDag);

	//Checks if it actually is a camera.
	if (camDag.node().apiType() == MFn::kCamera)
	{
		MFnCamera cam = camDag.node();
		//Fetches the parent of the camera which will always be a transformer
		MFnTransform camTransform = cam.parent(0);
		MMatrix viewMat = camTransform.transformation().asMatrix().matrix;
		MFloatMatrix projectionMat = cam.projectionMatrix();

		MString camName = camTransform.absoluteName();
		//MGlobal::displayInfo(MString("CameraName: " + camName));
		//
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[0][0] + " " + viewMat.matrix[0][1] + " " + viewMat.matrix[0][2] + " " + viewMat.matrix[0][3]);
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[1][0] + " " + viewMat.matrix[1][1] + " " + viewMat.matrix[1][2] + " " + viewMat.matrix[1][3]);
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[2][0] + " " + viewMat.matrix[2][1] + " " + viewMat.matrix[2][2] + " " + viewMat.matrix[2][3]);
		//MGlobal::displayInfo(MString("Cam:") + viewMat.matrix[3][0] + " " + viewMat.matrix[3][1] + " " + viewMat.matrix[3][2] + " " + viewMat.matrix[3][3]);
		//
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[0][0] + " " + projectionMat.matrix[0][1] + " " + projectionMat.matrix[0][2] + " " + projectionMat.matrix[0][3]);
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[1][0] + " " + projectionMat.matrix[1][1] + " " + projectionMat.matrix[1][2] + " " + projectionMat.matrix[1][3]);
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[2][0] + " " + projectionMat.matrix[2][1] + " " + projectionMat.matrix[2][2] + " " + projectionMat.matrix[2][3]);
		//MGlobal::displayInfo(MString("Cam:") + projectionMat.matrix[3][0] + " " + projectionMat.matrix[3][1] + " " + projectionMat.matrix[3][2] + " " + projectionMat.matrix[3][3]);

		double ARO = cam.aspectRatio();
		double HFOV = cam.horizontalFieldOfView();
		double VFOV = cam.verticalFieldOfView();
		double nPlane = cam.nearClippingPlane();
		double fPlane = cam.farClippingPlane();

		//MGlobal::displayInfo(MString("Cam:") + ARO);
		//MGlobal::displayInfo(MString("Cam:") + HFOV);
		//MGlobal::displayInfo(MString("Cam:") + VFOV);
		//MGlobal::displayInfo(MString("Cam:") + nPlane);
		//MGlobal::displayInfo(MString("Cam:") + fPlane);

		ComLib::CameraData camData;

		//ViewMat
		camData.viewMat[0][0] = viewMat.matrix[0][0];
		camData.viewMat[1][0] = viewMat.matrix[1][0];
		camData.viewMat[2][0] = viewMat.matrix[2][0];
		camData.viewMat[3][0] = viewMat.matrix[3][0];

		camData.viewMat[0][1] = viewMat.matrix[0][1];
		camData.viewMat[1][1] = viewMat.matrix[1][1];
		camData.viewMat[2][1] = viewMat.matrix[2][1];
		camData.viewMat[3][1] = viewMat.matrix[3][1];

		camData.viewMat[0][2] = viewMat.matrix[0][2];
		camData.viewMat[1][2] = viewMat.matrix[1][2];
		camData.viewMat[2][2] = viewMat.matrix[2][2];
		camData.viewMat[3][2] = viewMat.matrix[3][2];

		camData.viewMat[0][3] = viewMat.matrix[0][3];
		camData.viewMat[1][3] = viewMat.matrix[1][3];
		camData.viewMat[2][3] = viewMat.matrix[2][3];
		camData.viewMat[3][3] = viewMat.matrix[3][3];

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

		//comlib.send(&camData, ComLib::MSG_TYPE::CAMERA, sizeof(ComLib::CameraData));
	}

	MCallbackId undoId = MEventMessage::addEventCallback(
		"Undo",
		undoCallback,
		NULL,
		&res);
	if (MS::kSuccess != res)
		return MS::kFailure;
	myCallbackArray.append(undoId);

	MCallbackId nodeAddedId = MDGMessage::addNodeAddedCallback(
		nodeCreationCallback,
		kDefaultNodeType,
		NULL,
		&res);

	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(nodeAddedId) == MS::kSuccess) {};
	}

	MCallbackId nodeRemovedId = MDGMessage::addNodeRemovedCallback(
		nodeDeleteCallback,
		kDefaultNodeType,
		NULL,
		&res);

	if (res == MS::kSuccess)
	{
		if (myCallbackArray.append(nodeRemovedId) == MS::kSuccess) {};
	}
	
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