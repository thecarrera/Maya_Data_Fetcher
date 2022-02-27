#pragma once

#define NT_PLUGIN
#define REQUIRE_IOSTREAM
#define EXPORT __declspec(dllexport)

// Commands
#include <maya/MPxCommand.h>
#pragma comment(lib,"OpenMayaUI.lib")

// Libraries to link from Maya
// This can be also done in the properties setting for the project.
#pragma comment(lib,"Foundation.lib")
#pragma comment(lib,"OpenMaya.lib")

#include <maya/M3dView.h>

#include <maya/MFnPlugin.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>
#include <maya/MUuid.h>
#include <maya/MIOStream.h>
#include <maya/MFnSet.h>
#include <maya/MDagPath.h>

#include <maya/MFnCamera.h>

#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MPoint.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMatrixData.h>

#include <maya/MVector.h>
#include <maya/MFloatVector.h>
#include <maya/MMatrix.h>
#include <maya/MFloatMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnRotateManip.h>
#include <maya/MFnScaleManip.h>

#include <maya/MMaterial.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnBlinnShader.h>
#include <maya/MFnPhongShader.h>

#include <maya/MImage.h>

#include <maya/MFnPointLight.h>

#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshEdge.h>

#include <maya/MIntArray.h>
#include <maya/MPlugArray.h>
#include <maya/MPointArray.h>
#include <maya/MFloatPointArray.h>

// Wrappers
#include <maya/MGlobal.h>

// Messages
#include <maya/MMessage.h>
#include <maya/MTimerMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MEventMessage.h>
#include <maya/MPolyMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MDagPath.h>
#include <maya/MDagMessage.h>
#include <maya/MUiMessage.h>
#include <maya/MModelMessage.h>

