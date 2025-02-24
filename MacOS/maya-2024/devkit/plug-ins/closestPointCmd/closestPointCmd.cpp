//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+


// MAYA HEADERS
#include <maya/MFnPlugin.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MMeshIntersector.h>
#include <maya/MItSelectionList.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MArgList.h>
#include <maya/MFloatPoint.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPointLight.h>
#include <maya/MFloatPointArray.h>
#include <maya/MPxCommand.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>


#define MERR_CHK(status,msg) if ( !status ) { MGlobal::displayError(msg); } // cerr << msg << endl; }

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	PLUGIN DESCRIPTION
//	
//	This is an example of the closest point between a point light and a mesh.
//	
//
//	PLUGIN INSTRUCTIONS
//
//	- create a point light and a poly plane (mesh)
//	- run the command as such: closestPointCmd <pointLightName> <planeName>
//	- a sphere will be created at the closest point
//	
//	Use the following script to automatically display closest point on the mesh (assuming 
//	a point light and a mesh with the names used here exist):
//
//	global proc closestPointExample()
//	{
//		closestPointCmd pointLight1 pPlane1;
//		select -r pointLight1;
//	}
//
//	scriptJob -ac "pointLight1.tx" closestPointExample;
//	scriptJob -ac "pointLight1.ty" closestPointExample;
//	scriptJob -ac "pointLight1.tz" closestPointExample;
//
//	scriptJob -ac "pointLight1.rx" closestPointExample;
//	scriptJob -ac "pointLight1.ry" closestPointExample;
//	scriptJob -ac "pointLight1.rz" closestPointExample;
//

/*

// Alternatively use the following code to demonstrate
// the closest point
//

	loadPlugin closestPointCmd;

	file -f -new;
	defaultPointLight(1, 1,1,1, 0, 0, 0,0,0, 1);
	move -r 0 5 0 ;
	polyPlane -w 1 -h 1 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1;
	scale -r 10 10 10 ;
	closestPointCmd pointLight1 pPlane1;

	file -f -new;
	defaultPointLight(1, 1,1,1, 0, 0, 0,0,0, 1);
	move -r 2 5 0 ;
	polyPlane -w 1 -h 1 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1;
	scale -r 10 10 10 ;
	closestPointCmd pointLight1 pPlane1;
	select -cl;

	unloadPlugin closestPointCmd ;

*/


//////////////////////////////////////////////////////////////////////////////////////////////////


// MAIN CLASS FOR THE closestPointCmd COMMAND:
class closestPointCmd : public MPxCommand
{
	public:
		closestPointCmd();
		~closestPointCmd() override;
		static void* creator();
		bool isUndoable() const override;
		MStatus doIt(const MArgList&) override;
		MStatus undoIt() override;

		void createDisplay( MPointOnMesh& info, MMatrix& matrix);
};



// CONSTRUCTOR:
closestPointCmd::closestPointCmd()
{
}



// DESTRUCTOR:
closestPointCmd::~closestPointCmd()
{
}



// FOR CREATING AN INSTANCE OF THIS COMMAND:
void* closestPointCmd::creator()
{
   return new closestPointCmd;
}



// MAKE THIS COMMAND NOT UNDOABLE:
bool closestPointCmd::isUndoable() const
{
   return false;
}

MStatus closestPointCmd::doIt(const MArgList& args)
// Description:
// 		Determine the closest point between the point
//		light and the mesh.
//		If it does, it displays the point.
{
	MStatus status = MStatus::kSuccess;

	if (args.length() != 2) 
	{
		MGlobal::displayError("Need 2 items!");
		return MStatus::kFailure;
	}

	MSelectionList activeList;
	int i;
	for ( i = 0; i < 2; i++)
	{
		MString strCurrSelection;
		status = args.get(i, strCurrSelection);
		if (MStatus::kSuccess == status) activeList.add(strCurrSelection);
	}


	MItSelectionList iter(activeList);
	MFnPointLight fnLight;  
	MFnDagNode dagNod;
	MFnDependencyNode fnDN;
	MDagPath pathToMesh;

	float fX = 0;
	float fY = 0;
	float fZ = 0;

	for ( ; !iter.isDone(); iter.next() )
	{
		MObject tempObjectParent, tempObjectChild;
		iter.getDependNode(tempObjectParent);

		if (tempObjectParent.apiType() == MFn::kTransform)
		{
			dagNod.setObject(tempObjectParent);
			tempObjectChild = dagNod.child(0, &status);
		}

		// check what type of object is selected
		if (tempObjectChild.apiType() == MFn::kPointLight)
		{
			MDagPath pathToLight;
			MERR_CHK(MDagPath::getAPathTo(tempObjectParent, pathToLight), 
					"Couldn't get a path to the pointlight");
			MERR_CHK(fnLight.setObject(pathToLight), "Failure on assigning light");

			status = fnDN.setObject(tempObjectParent);

			MPlug pTempPlug = fnDN.findPlug("translateX",  true,  &status);
			if (MStatus::kSuccess == status)
			{
				pTempPlug.getValue(fX);
			}

			pTempPlug = fnDN.findPlug("translateY",  true,  &status);
			if (MStatus::kSuccess == status)
			{
				pTempPlug.getValue(fY);
			}

			pTempPlug = fnDN.findPlug("translateZ",  true,  &status);
			if (MStatus::kSuccess == status)
			{
				pTempPlug.getValue(fZ);
			}	
		}
		else if (tempObjectChild.apiType() == MFn::kMesh)
		{
			MERR_CHK(MDagPath::getAPathTo(tempObjectChild, pathToMesh), 
				"Couldn't get a path to the pointlight");
		}
		else
		{
			MGlobal::displayError("Need a pointlight and a mesh");
			return MStatus::kFailure;
		}
	}

	MMeshIntersector intersector;
	MMatrix matrix = pathToMesh.inclusiveMatrix();
	MObject node = pathToMesh.node();
	status = intersector.create( node, matrix );
	if ( status )
	{
		MPoint point(fX,fY,fZ);
		MPointOnMesh pointInfo;
		cout << "Using point: " << point << endl;
		status = intersector.getClosestPoint( point, pointInfo );
		if ( status )
		{
			createDisplay( pointInfo, matrix );
		}
		else
		{
			MGlobal::displayError("Failed to get closest point");
		}
	}
	else
	{
		MGlobal::displayError("Failed to create intersector");
	}

	return status;
}

// UNDO THE COMMAND
MStatus closestPointCmd::undoIt()
{
	MStatus status;
	// undo not implemented
	return status;
}

void closestPointCmd::createDisplay( MPointOnMesh& info, MMatrix& matrix )
{
	MPoint worldPoint( info.getPoint() );
	worldPoint = worldPoint * matrix;
	MVector worldNormal( info.getNormal() );
	worldNormal = worldNormal * matrix;
	worldNormal.normalize();

	MString strCommandString = "string $strBall[] = `polySphere -r 0.5`;";
	strCommandString += "$strBallName = $strBall[0];";
	strCommandString += "setAttr ($strBallName + \".tx\") ";
	strCommandString += worldPoint.x;
	strCommandString += ";";
	strCommandString += "setAttr ($strBallName + \".ty\") ";
	strCommandString += worldPoint.y;
	strCommandString += ";";
	strCommandString += "setAttr ($strBallName + \".tz\") ";
	strCommandString += worldPoint.z;
	strCommandString += ";";

	MGlobal::executeCommand(strCommandString);

	// Other info
	cout << "Normal: " << worldNormal << " face id: " << info.faceIndex() << 
		" triangle id: " << info.triangleIndex() << endl;
}

// INITIALIZE THE PLUGIN:
MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "9.0", "Any");
	status = plugin.registerCommand("closestPointCmd", closestPointCmd::creator);
	return status;
}

// UNINITIALIZE THE PLUGIN:
MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);
	plugin.deregisterCommand("closestPointCmd");
	return status;
}

