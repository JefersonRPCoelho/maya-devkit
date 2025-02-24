#ifndef _pointOnSubdNode
#define _pointOnSubdNode
//
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

// File: pointOnSubdNode.h
//
// Dependency Graph Node: pointOnSubd
//
// pointOnSubdNode
// 
// Produces dependency graph node pointOnSubd
// 
// This node is a simple example of how to query subdivision surface as an
// input to a dependency node.  This node takes a subdivision surface and a
// parameter point on subdivision surface and outputs the position and the
// normal of the surface at that point.
// 
// One of the inputs for this node is called "subd".  Subdivision surface
// shape nodes ("subdiv") have two compatible output attributes that you
// can use as inputs for pointOnSubd node - outSubdiv and worldSubdiv.
// 
// Another four numerical inputs are needed to specify the parameter point
// on the subdivision surface.  The two integer values ("faceFirst" and
// "faceSecond") specify a face.  These two values are the same ones that
// you see as a part of the selection description when you select a
// subdivision surface face.  The two floating point values ("uValue" and
// "vValue") specify the position within that face.
// 
// A boolean attribute "relative" specifies if the "uValue" and "vValue"
// are given in the 0-1 range ("relative" is true) or in the U-V range of
// the specified subdivision surface face ("relative" is false).
// 
// To run this example, create a subdivision surface (converting from a
// polygon, for example), create another piece of geometry (NURBS cone, for
// example) and group the subdivision surface and the other geometry.  Your
// other geometry should be oriented Y-up.  Now, select the other geometry,
// followed by a single face on the subdivision surface.  A face at any
// level is available.  With both of those items selected, type:
// 
// connectObjectToPointOnSubd
// 
// and the object will "stick" the the middle of the subdivision surface
// face.  You can position it elsewhere using the attribute editor for the
// newly created pointOnSubd node and changing the U and V parameters.
// 
// An actual MEL running example is provided in the
// "connectObjectToPointOnSubd.mel" script file.  You need to cut and
// paste into the script editor and run it.


#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MTypeId.h> 

 
class pointOnSubd : public MPxNode
{
public:
						pointOnSubd();
					~pointOnSubd() override; 

	MStatus		compute( const MPlug& plug, MDataBlock& data ) override;

	static  void*		creator();
	static  MStatus		initialize();

public:

	// There needs to be a MObject handle declared for each attribute that
	// the node will have.  These handles are needed for getting and setting
	// the values later.
	//
	static  MObject		aSubd;
	static  MObject		aFaceFirst;
	static  MObject		aFaceSecond;
	static 	MObject		aRelativeUV;
	static  MObject		aU;
	static  MObject		aV;

	static  MObject		aPoint;
	static  MObject		aPointX;
	static  MObject		aPointY;
	static  MObject		aPointZ;
	static  MObject		aNormal;
	static  MObject		aNormalX;
	static  MObject		aNormalY;
	static  MObject		aNormalZ;


	// The typeid is a unique 32bit indentifier that describes this node.
	// It is used to save and retrieve nodes of this type from the binary
	// file format.  If it is not unique, it will cause file IO problems.
	//
	static	MTypeId		id;
};

#endif
