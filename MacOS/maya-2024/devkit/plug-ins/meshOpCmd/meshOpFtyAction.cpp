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

#include "meshOpFty.h"

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFloatPointArray.h>

// Function Sets
//
#include <maya/MFnMesh.h>

// Iterators
//
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshEdge.h>

#define CHECK_STATUS(st) if ((st) != MS::kSuccess) { break; }

MStatus meshOpFty::doIt()
//
//	Description:
//		Performs the operation on the selected mesh and components
//
{
	MStatus status;
	unsigned int i, j;

	// Get access to the mesh's function set
	//
	MFnMesh meshFn(fMesh);

	// The division count argument is used in many of the operations
	// to execute the operation multiple subsequent times. For example,
	// with a division count of 2 in subdivide face, the given faces will be
	// divide once and then the resulting inner faces will be divided again.
	//
	int divisionCount = 2;

	MFloatVector translation;
	if (fOperationType == kExtrudeEdges
		|| fOperationType == kExtrudeFaces
		|| fOperationType == kDuplicateFaces
		|| fOperationType == kExtractFaces)
	{
		// The translation vector is used for the extrude, extract and 
		// duplicate operations to move the result to a new position. For 
		// example, if you extrude an edge on a mesh without a subsequent 
		// translation, the extruded edge will be on at the position of the 
		// orignal edge and the created faces will have no area.
		// 
		// Here, we provide a translation that is in the same direction as the
		// average normal of the given components.
		//
		MFn::Type componentType = getExpectedComponentType(fOperationType);
		MIntArray adjacentVertexList;
		switch (componentType)
		{
		case MFn::kMeshEdgeComponent:
			for (i = 0; i < fComponentIDs.length(); ++i)
			{
				int2 vertices;
				meshFn.getEdgeVertices(fComponentIDs[i], vertices);
				adjacentVertexList.append(vertices[0]);
				adjacentVertexList.append(vertices[1]);
			}
			break;

		case MFn::kMeshPolygonComponent:
			for (i = 0; i < fComponentIDs.length(); ++i)
			{
				MIntArray vertices;
				meshFn.getPolygonVertices(fComponentIDs[i], vertices);
				for (j = 0; j < vertices.length(); ++j)
					adjacentVertexList.append(vertices[j]);
			}
			break;
		default:	
			break;
		}
		MVector averageNormal(0, 0, 0);
		for (i = 0; i < adjacentVertexList.length(); ++i)
		{
			MVector vertexNormal;
			meshFn.getVertexNormal(adjacentVertexList[i], true, vertexNormal,
				MSpace::kWorld);
			averageNormal += vertexNormal;
		}
		if (averageNormal.length() < 0.001)
			averageNormal = MVector(0.0, 1.0, 0.0);
		else averageNormal.normalize();
		translation = averageNormal;
	}

	// When doing an extrude operation, there is a choice of extrude the
	// faces/edges individually or together. If extrudeTogether is true and 
	// multiple adjacent components are selected, they will be extruded as if
	// it were one more complex component.
	//
	// The following variable sets that option.
	//
	bool extrudeTogether = true;

	// Execute the requested operation
	//
	switch (fOperationType)
	{
	case kSubdivideEdges: {
		status = meshFn.subdivideEdges(fComponentIDs, divisionCount);
		CHECK_STATUS(status);
		break; }

	case kSubdivideFaces: {
		status = meshFn.subdivideFaces(fComponentIDs, divisionCount);
		CHECK_STATUS(status);
		break; }

	case kExtrudeEdges: {
		status = meshFn.extrudeEdges(fComponentIDs, divisionCount,
			&translation, extrudeTogether);
		CHECK_STATUS(status);
		break; }

	case kExtrudeFaces: {
		status = meshFn.extrudeFaces(fComponentIDs, divisionCount,
			&translation, extrudeTogether);
		CHECK_STATUS(status);
		break; }

	case kCollapseEdges: {
		status = meshFn.collapseEdges(fComponentIDs);
		CHECK_STATUS(status);
		break; }

	case kCollapseFaces: {
		status = meshFn.collapseFaces(fComponentIDs);
		CHECK_STATUS(status);
		break; }

	case kDuplicateFaces: {
		status = meshFn.duplicateFaces(fComponentIDs, &translation);
		CHECK_STATUS(status);
		break; }

	case kExtractFaces: {
		status = meshFn.extractFaces(fComponentIDs, &translation);
		CHECK_STATUS(status);
		break; }

	case kSplitLightning: {
		status = doLightningSplit(meshFn);
		CHECK_STATUS(status);
		break; }

	default:
		status = MS::kFailure;
		break;
	}

	return status;
}

MStatus meshOpFty::doLightningSplit(MFnMesh& meshFn)
//
//	Description:
//		Performs the kSplitLightning operation on the selected mesh
//      and components. It may not split all the selected components.
//
{
	unsigned int i, j;

	// These are the input arrays to the split function. The following
	// algorithm fills them in with the arguments for a continuous
	// split that goes through some of the selected faces.
	//
	MIntArray placements;
	MIntArray edgeIDs;
	MFloatArray edgeFactors;
	MFloatPointArray internalPoints;
	
	// The following array is going to be used to determine which faces
	// have been split. Since the split function can only split faces
	// which are adjacent to the earlier face, we may not split
	// all the faces
	//
	bool* faceTouched = new bool[fComponentIDs.length()];
	for (i = 0; i < fComponentIDs.length(); ++i)
		faceTouched[i] = false;
	
	// We need a starting point. For this example, the first face in
	// the component list is picked. Also get a polygon iterator
	// to this face.
	// 
	MItMeshPolygon itPoly(fMesh);
	for (; !itPoly.isDone(); itPoly.next())
	{
		if (fComponentIDs[0] == (int)itPoly.index()) break;
	}
	if (itPoly.isDone())
	{
		// Should never happen.
		//
		delete [] faceTouched;
		return MS::kFailure;
	}
	
	// In this example, edge0 is called the starting edge and
	// edge1 is called the destination edge. This algorithm will split
	// each face from the starting edge to the destination edge
	// while going through two inner points inside each face.
	//
	int edge0, edge1;
	MPoint innerVert0, innerVert1;
	int nextFaceIndex = 0;
	
	// We need a starting edge. For this example, the first edge in the
	// edge list is used.
	//
	MIntArray edgeList;
	itPoly.getEdges(edgeList);
	edge0 = edgeList[0];
	
	bool done = false;
	while (!done)
	{
		// Set this face as touched so that we don't try to split it twice
		//
		faceTouched[nextFaceIndex] = true;
		
		// Get the current face's center. It is used later in the
		// algorithm to calculate inner vertices.
		//
		MPoint faceCenter = itPoly.center();
			
		// Iterate through the connected faces to find an untouched,
		// selected face and get the ID of the shared edge. That face
		// will become the next face to be split.
		//
		MIntArray faceList;
		itPoly.getConnectedFaces(faceList);
		nextFaceIndex = -1;
		for (i = 0; i < fComponentIDs.length(); ++i)
		{
			for (j = 0; j < faceList.length(); ++j)
			{
				if (fComponentIDs[i] == faceList[j] && !faceTouched[i])
				{
					nextFaceIndex = i;
					break;
				}
			}
			if (nextFaceIndex != -1) break;
		}
		
		if (nextFaceIndex == -1)
		{
			// There is no selected and untouched face adjacent to this
			// face, so this algorithm is done. Pick the first edge that
			// is not the starting edge as the destination edge.
			//
			done = true;
			edge1 = -1;
			for (i = 0; i < edgeList.length(); ++i)
			{
				if (edgeList[i] != edge0)
				{
					edge1 = edgeList[i];
					break;
				}
			}
			if (edge1 == -1)
			{
				// This should not happen, since there should be more than
				// one edge for each face
				//
				delete [] faceTouched;
				return MS::kFailure;
			}
		}
		else
		{
			// The next step is to find out which edge is shared between
			// the two faces and use it as the destination edge. To do
			// that, we need to iterate through the faces and get the
			// next face's list of edges.
			//
			itPoly.reset();
			for (; !itPoly.isDone(); itPoly.next())
			{
				if (fComponentIDs[nextFaceIndex] == (int)itPoly.index()) break;
			}
			if (itPoly.isDone()) 
			{
				// Should never happen.
				//
				delete [] faceTouched;
				return MS::kFailure;
			}
			
			// Look for a common edge ID in the two faces edge lists
			//
			MIntArray nextFaceEdgeList;
			itPoly.getEdges(nextFaceEdgeList);
			edge1 = -1;
			for (i = 0; i < edgeList.length(); ++i)
			{
				for (j = 0; j < nextFaceEdgeList.length(); ++j)
				{
					if (edgeList[i] == nextFaceEdgeList[j])
					{
						edge1 = edgeList[i];
						break;
					}
				}
				if (edge1 != -1) break;
			}
			if (edge1 == -1)
			{
				// Should never happen.
				//
				delete [] faceTouched;
				return MS::kFailure;
			}
			
			// Save the edge list for the next iteration
			//
			edgeList = nextFaceEdgeList;
		}
		
		// Calculate the two inner points that the split will go through.
		// For this example, the midpoints between the center and the two
		// farthest vertices of the edges are used.
		//
		// Find the 3D positions of the edges' vertices
		//
		MPoint edge0vert0, edge0vert1, edge1vert0, edge1vert1;
		MItMeshEdge itEdge(fMesh, MObject::kNullObj );
		for (; !itEdge.isDone(); itEdge.next())
		{
			if (itEdge.index() == edge0)
			{
				edge0vert0 = itEdge.point(0);
				edge0vert1 = itEdge.point(1);
			}
			if (itEdge.index() == edge1)
			{
				edge1vert0 = itEdge.point(0);
				edge1vert1 = itEdge.point(1);
			}
		}
		
		// Figure out which are the farthest from each other
		//
		double distMax = edge0vert0.distanceTo(edge1vert0);
		MPoint max0, max1;
		max0 = edge0vert0;
		max1 = edge1vert0;
		double newDist = edge0vert1.distanceTo(edge1vert0);
		if (newDist > distMax)
		{
			max0 = edge0vert1;
			max1 = edge1vert0;
			distMax = newDist;
		}
		newDist = edge0vert0.distanceTo(edge1vert1);
		if (newDist > distMax)
		{
			max0 = edge0vert0;
			max1 = edge1vert1;
			distMax = newDist;
		}
		newDist = edge0vert1.distanceTo(edge1vert1);
		if (newDist > distMax)
		{
			max0 = edge0vert1;
			max1 = edge1vert1;
		}
		
		// Calculate the two inner points
		//
		innerVert0 = (faceCenter + max0) / 2.0;
		innerVert1 = (faceCenter + max1) / 2.0;
		
		// Add this split's information to the input arrays. If this is
		// the last split, also add the destination edge's split information.
		//
		placements.append((int) MFnMesh::kOnEdge);
		placements.append((int) MFnMesh::kInternalPoint);
		placements.append((int) MFnMesh::kInternalPoint);
		if (done) placements.append((int) MFnMesh::kOnEdge);
		
		edgeIDs.append(edge0);
		if (done) edgeIDs.append(edge1);
		
		edgeFactors.append(0.5f);
		if (done) edgeFactors.append(0.5f);
		
		MFloatPoint point1((float)innerVert0[0], (float)innerVert0[1],
			(float)innerVert0[2], (float)innerVert0[3]);
		MFloatPoint point2((float)innerVert1[0], (float)innerVert1[1],
			(float)innerVert1[2], (float)innerVert1[3]);
		internalPoints.append(point1);
		internalPoints.append(point2);
		
		// For the next iteration, the current destination
		// edge becomes the start edge.
		//
		edge0 = edge1;
	}

	// Release the dynamically-allocated memory and do the actual split
	//
	delete [] faceTouched;
	return meshFn.split(placements, edgeIDs, edgeFactors, internalPoints);
}
