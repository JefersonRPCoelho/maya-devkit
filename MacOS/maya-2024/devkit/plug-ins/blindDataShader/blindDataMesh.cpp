//-
// ==========================================================================
// Copyright 2018 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+


#include <maya/MFnMesh.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MFnMeshData.h>
#include <maya/MIOStream.h>
#include <maya/MItMeshVertex.h>

#include "blindDataMesh.h"

MStatus returnStatus;

#define McheckErr(stat,msg)			\
	if ( MS::kSuccess != stat ) {	\
		cerr << msg;				\
		return MS::kFailure;		\
	}

MObject blindDataMesh::seed;
MObject blindDataMesh::outputMesh;
MTypeId blindDataMesh::id( 0x60EA );

void* blindDataMesh::creator()
{
	return new blindDataMesh;
}

MStatus blindDataMesh::initialize()
{
	MFnTypedAttribute typedAttr;
	MStatus returnStatus;

	blindDataMesh::outputMesh = typedAttr.create( "outputMesh", "out",
		MFnData::kMesh, MObject::kNullObj, &returnStatus ); 
	McheckErr(returnStatus, "ERROR creating blindDataMesh output attribute\n");
	typedAttr.setStorable(false);

	returnStatus = addAttribute(blindDataMesh::outputMesh);
	McheckErr(returnStatus, "ERROR adding outputMesh attribute\n");

	MFnNumericAttribute numAttr;
	blindDataMesh::seed = numAttr.create( "randomSeed", "seed",
		MFnNumericData::kLong, 0, &returnStatus );
	McheckErr(returnStatus, "ERROR creating blindDataMesh input attribute\n");

	returnStatus = addAttribute(blindDataMesh::seed);
	McheckErr(returnStatus, "ERROR adding input attribute\n");

	returnStatus = attributeAffects(blindDataMesh::seed,
								    blindDataMesh::outputMesh);
	McheckErr(returnStatus, "ERROR in attributeAffects\n");

	return MS::kSuccess;
}

MObject blindDataMesh::createMesh(long seed, MObject& outData, MStatus& stat)
{
	MFloatPointArray vertices;
	MIntArray faceDegrees;
	MIntArray faceVertices;
	int i, j;

	srand(seed);

	float planeSize = 20.0f;
	float planeOffset = planeSize / 2.0f;
	float planeDim = 0.5f;

	int numDivisions = (int) (planeSize / planeDim);
	// int numVertices = (numDivisions + 1) * (numDivisions + 1);
	// int numEdge = (2 * numDivisions) * (numDivisions + 1);
	int numFaces = numDivisions * numDivisions;

	// Set up an array containing the vertex positions for the plane. The
	// vertices are placed equi-distant on the X-Z plane to form a square
	// grid that has a side length of "planeSize".
	//
	// The Y-coordinate of each vertex is the average of the neighbors already
	// calculated, if there are any, with a small random offset added. Because
	// of the way the vertices are calculated, the whole plane will look like
	// it is streaked in a diagonal direction with mountains and depressions.
	//
	for (i = 0; i < (numDivisions + 1); ++i)
	{
		for (j = 0; j < (numDivisions + 1); ++j)
		{
			float height;

			if (i == 0 && j == 0)
			{
				height = ((rand() % 101) / 100.0f - 0.5f);
			}
			else if (i == 0)
			{
				float previousHeight = vertices[j - 1][1];
				height = previousHeight + ((rand() % 101) / 100.0f - 0.5f);
			}
			else if (j == 0)
			{
				float previousHeight = vertices[(i-1)*(numDivisions + 1)][1];
				height = previousHeight + ((rand() % 101) / 100.0f - 0.5f);
			}
			else
			{
				float previousHeight
					= vertices[(i-1)*(numDivisions + 1) + j][1];
				float previousHeight2
					= vertices[i*(numDivisions + 1) + j - 1][1];
				height = (previousHeight + previousHeight2)
					/ 2.0f + ((rand() % 101) / 100.0f - 0.5f);
			}

			MFloatPoint vtx( i * planeDim - planeOffset, height,
				j * planeDim - planeOffset );
			vertices.append(vtx);
		}
	}

	// Set up an array containing the number of vertices
	// for each of the plane's faces
	//
	for (i = 0; i < numFaces; ++i)
	{
		faceDegrees.append(4);
	}

	// Set up an array to assign the vertices for each face
	//
	for (i = 0; i < numDivisions; ++i)
	{
		for (j = 0; j < numDivisions; ++j)
		{
			faceVertices.append(i*(numDivisions+1) + j);
			faceVertices.append(i*(numDivisions+1) + j + 1);
			faceVertices.append((i+1)*(numDivisions+1) + j + 1);
			faceVertices.append((i+1)*(numDivisions+1) + j);
		}
	}

	MFnMesh	meshFn;
	MObject newMesh = meshFn.create(vertices.length(), numFaces, vertices,
		faceDegrees, faceVertices, outData, &stat);

	return newMesh;
}

MStatus blindDataMesh::compute(const MPlug& plug, MDataBlock& data)

{
	MStatus returnStatus;

	if (plug == outputMesh) {

		// Get the given random number generator seed. We need to use a
		// seed, because a pseudo-random number generator will always give
		// the same random numbers for a constant seed. This means that the
		// mesh will not change when it is recalculated.
		//
		MDataHandle seedHandle = data.inputValue(seed, &returnStatus);
		McheckErr(returnStatus,"ERROR getting random number generator seed\n");

		long seed = seedHandle.asInt();

		// Get the handle to the output mesh. The creation of the output mesh
		// is done in two steps. First, the mesh is created. That involves
		// calculating the position of the vertices and their connectivity.
		//
		// Second, blind data is associated to the vertices on the mesh.
		// For this example, three double blind data values is associated
		// to each vertex: "red", "green" and "blue".
		//
		MFnMeshData dataCreator;
		MDataHandle outputHandle = data.outputValue(outputMesh, &returnStatus);
		McheckErr(returnStatus, "ERROR getting polygon data handle\n");
		MObject newOutputData = dataCreator.create(&returnStatus);
		McheckErr(returnStatus, "ERROR creating outputData");
		createMesh(seed, newOutputData, returnStatus);
		McheckErr(returnStatus, "ERROR creating new plane");
		returnStatus = setMeshBlindData(newOutputData);
		McheckErr(returnStatus, "ERROR setting the blind Data on the plane");

		outputHandle.set(newOutputData);
		data.setClean( plug );
	} else
		return MS::kUnknownParameter;

	return MS::kSuccess;
}

MStatus blindDataMesh::setMeshBlindData(MObject& mesh)
{
	MStatus stat = MS::kSuccess;
	MFnMesh meshFn(mesh);

	// First, make sure that the blind data attribute exists,
	// Otherwise, create it.
	//
	int blindDataID = 60;
	if (!meshFn.isBlindDataTypeUsed(blindDataID, &stat))
	{
		MStringArray longNames, shortNames, formatNames;
		
		longNames.append("red_color");
		shortNames.append("red");
		formatNames.append("double");

		longNames.append("green");
		shortNames.append("grn");
		formatNames.append("double");
		
		longNames.append("blue_color");
		shortNames.append("blue");
		formatNames.append("double");

		stat = meshFn.createBlindDataType(
			blindDataID, longNames, shortNames, formatNames );
		if (!stat) return stat;
	}
	else if (!stat) return stat;

	// Iterate through the mesh vertices and assign to each some
	// color value that is related to the height of the vertex
	// so that it goes from dark blue at the lowest point to
	// white at the highest..
	//
	// Find the lowest and the highest points.
	//
	MItMeshVertex itVertex(mesh);
	double lowest = 1e10, highest = -1e10;
	for ( ; !itVertex.isDone(); itVertex.next() )
	{
		MPoint vertexPosition = itVertex.position();
		double height = vertexPosition[1];
		if (height < lowest) lowest = height;
		if (height > highest) highest = height;
	}

	double range = highest - lowest;
	for ( itVertex.reset(mesh); !itVertex.isDone(); itVertex.next() )
	{
		MPoint vertexPosition = itVertex.position();
		double height = vertexPosition[1] - lowest;
		double red, green, blue;

		// Calculate the interpolated color for each vertex
		// using its relative height
		//
		red = 2.0 * ( height / range ) - 1.0;
		if (height > range/2.0) {
			if (red < 0.7) green = 0.7;
			else green = red;
		}
		else green = 0.7 * (1.0 - (((range/2.0) - height) / (range/2.0))
			* (((range/2.0) - height) / (range/2.0)));
		if (height > range/2.0) blue = red;
		else blue = 1.0 - (green*green);
		if (red < 0.0) red = 0.0;

		// Set the color values in the blind data
		//
		stat = meshFn.setDoubleBlindData(itVertex.index(),
			MFn::kMeshVertComponent, blindDataID, "red", red);
		if (!stat) return stat;
		stat = meshFn.setDoubleBlindData(itVertex.index(),
			MFn::kMeshVertComponent, blindDataID, "green", green);
		if (!stat) return stat;
		stat = meshFn.setDoubleBlindData(itVertex.index(),
			MFn::kMeshVertComponent, blindDataID, "blue", blue);
		if (!stat) return stat;
	}

	return stat;
}
