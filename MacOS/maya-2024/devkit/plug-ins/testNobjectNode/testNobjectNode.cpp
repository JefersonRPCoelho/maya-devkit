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

#include "testNobjectNode.h"
#include <maya/MIOStream.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MDagPath.h>
#include <maya/MPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MItMeshVertex.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnNObjectData.h>
#include "stdio.h"

/*
Introduction to interacting with the N Solver
=============================================

In order to create an N cloth object that can interact with the Nucleus solver, 
your object will need to own a MnCloth, which represents the underlying
N cloth and its data.
your node will also need the following 6 attributes:

ATTR                Type        Description
startState          kNObject    initial state of your N object
currentState        kNObject    current state of your N object
currentTime         Time        connection to the current time
nextState           kNObject    next state of you N object
inputGeom           kMesh       input mesh       
outputGeom          kMesh       output mesh

inputGeom,outputGeom and currentTime are self explanatory.

A connection is to be made from the nucleus solver's outputObjects attribute to 
the nextState attribute of your node.  Also, you need to connect the currentState
and startState attributes of your node to the inputActive and inputActiveStart attributes
on the solver node respectively.

Once these connections are made, the normal sequence of events is the following:

The refresh will trigger a pull on the output mesh attribute.  At this your node will
pull on the nextState attribute, triggering a solve from the solver.  Depending on the current
time, the solver will trigger pulls on either the currentState or startState attributes of your
node.  If the startState is pulled on, you need to initialize the MnCloth which you node
owns from the input geometry.  Once this is done and the data passed back to the solver,
a solve will occur, and the solver will automatically update your MnCloth behind the scenes.

At this point you may extract the results of the solve via methods on the MnCloth and apply it
to the output mesh.

Below is a script that show how to test this node:

// This example show how 2 cloth objects falling and colliding with a sphere
// side by side.  One is a default nCloth object, the other is a cloth
// object created by our plugin.

//---------------------------------------------------------------------------------
// Note: Before running this code, make sure the plugin testNobjectNode is loaded!
//---------------------------------------------------------------------------------
global proc setupCustomClothScene()
{
    file -f -new;
    //plane1 will be driven by regular nCloth
    string $pPlane1[] = `polyPlane -w 5 -h 5 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1`;
    move -r -10 0 0;
    createNCloth 0;

    //plane2 will act as input to our testNobjectNode
    string $pPlane2[] = `polyPlane -w 5 -h 5 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1`;
    //plane3 will act as output to our testNobjectNode
    string $pPlane3[] = `polyPlane -w 5 -h 5 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1`;
    select -r polyPlane3 ;
    doDelete;


    //spheres 1 and 2 will both act as passive objects.
    string $pSphere1[] = `polySphere -r 1 -sx 20 -sy 20 -ax 0 1 0 -cuv 2 -ch 1`;
    move -r -10 -3 0;
    string $pSphere2[] = `polySphere -r 1 -sx 20 -sy 20 -ax 0 1 0 -cuv 2 -ch 1`;
    move -r 0 -3 0;
    select -r $pSphere1[0] $pSphere2[0];
    makeCollideNCloth;

    createNode testNobjectNode;
    connectAttr pPlaneShape2.worldMesh[0] testNobjectNode1.inputGeom;
    connectAttr nucleus1.outputObjects[1] testNobjectNode1.nextState;
    connectAttr testNobjectNode1.currentState nucleus1.inputActive[1];
    connectAttr testNobjectNode1.startState nucleus1.inputActiveStart[1];
    connectAttr testNobjectNode1.outputGeom pPlaneShape3.inMesh;
    connectAttr time1.outTime testNobjectNode1.currentTime;
}

*/

const MTypeId testNobjectNode::id( 0x85003 );

MObject testNobjectNode::startState;
MObject testNobjectNode::currentState;
MObject testNobjectNode::currentTime;
MObject testNobjectNode::nextState;
MObject testNobjectNode::inputGeom;
MObject testNobjectNode::outputGeom;

inline void statCheck( MStatus stat, MString msg )
{
	if ( !stat )
	{
		cout<<msg<<"\n";
	}
}

testNobjectNode::testNobjectNode()
{
    //Create my N cloth.
    fNObject.createNCloth();
}

MStatus testNobjectNode::compute(const MPlug &plug, MDataBlock &data)
{
	MStatus stat;

	if ( plug == outputGeom )
	{
		MObject inMeshObj = data.inputValue(inputGeom).asMesh();
		
        cerr<<"pull on outputGeom\n";

		MFnMeshData meshDataFn;
        MFnMesh inputMesh(inMeshObj);
		MObject newMeshObj = meshDataFn.create();
		MFnMesh newMeshFn;
		newMeshFn.copy( inMeshObj, newMeshObj );        

        //get the value of the currentTime so it can correctly dirty the
        //startState, currentState.
        data.inputValue(currentTime).asTime();

		// pull on next state.  This will cause the solver to pull on either
        // the startState or the currentState, depending on the time of solve.
        // When we return the state to the solver, it will do the solve and update
        // The N Object data directly.
		MObject nextNObj = data.inputValue(nextState).data();
        MFloatPointArray pts;        

        //At this point the N Object's internal state should have been updated
        //by the solver.  Read it out and set the output mesh.
        fNObject.getPositions(pts);
        if(pts.length() == (unsigned int) inputMesh.numVertices()) {
            newMeshFn.setPoints(pts);
        }

		newMeshFn.setObject( newMeshObj );

		data.outputValue(outputGeom).set(newMeshObj);
		data.setClean(plug);

	}
	if ( plug == currentState )
	{               
        MFnNObjectData outputData;
        MObject mayaNObjectData = outputData.create();
        outputData.setObject(mayaNObjectData);
        
        outputData.setObjectPtr(&fNObject);        
        outputData.setCached(false);

        MDataHandle currStateOutputHandle = data.outputValue(currentState);
        currStateOutputHandle.set(outputData.object());
	  
        cerr<<"pull on currentState\n";
	}
	if ( plug == startState )
	{
	    int ii,jj;
        // initialize MnCloth
        MObject inMeshObj = data.inputValue(inputGeom).asMesh();
				
        MFnMesh inputMesh(inMeshObj);		

        int numPolygons = inputMesh.numPolygons();
        int * faceVertCounts = new int[numPolygons];
                
        
        int facesArrayLength = 0;
        for(ii=0;ii<numPolygons;ii++) {
            MIntArray verts;
            inputMesh.getPolygonVertices(ii,verts);
            faceVertCounts[ii] = verts.length();
            facesArrayLength += verts.length();
        }
        int * faces = new int[facesArrayLength];
        int currIndex = 0;
        for(ii=0;ii<numPolygons;ii++) {
            MIntArray verts;
            inputMesh.getPolygonVertices(ii,verts);
            for(jj=0;jj<(int)verts.length();jj++) {
                faces[currIndex++] = verts[jj];
            }
        }

        int numEdges = inputMesh.numEdges();
        int * edges = new int[2*numEdges];
        currIndex = 0;
        for(ii=0;ii<numEdges;ii++) {
            int2 edge;
            inputMesh.getEdgeVertices(ii,edge);
            edges[currIndex++] = edge[0];
            edges[currIndex++] = edge[1];
        }

        // When you are doing the initialization, the first call must to be setTopology().  All other
        // calls must come after this.
        fNObject.setTopology(numPolygons, faceVertCounts, faces,numEdges, edges );
        delete[] faceVertCounts;
        delete[] faces;
        delete[] edges;        


        unsigned int numVerts = 0;
        numVerts = inputMesh.numVertices();        

        MFloatPointArray vertexArray;
        inputMesh.getPoints(vertexArray);
        fNObject.setPositions(vertexArray,true);

        MFloatPointArray velocitiesArray;
        velocitiesArray.setLength(numVerts);
        for(ii=0;ii<(int)numVerts;ii++) {
            velocitiesArray[ii].x = 0.0f;
            velocitiesArray[ii].y = 0.0f;
            velocitiesArray[ii].z = 0.0f;
            velocitiesArray[ii].w = 0.0f;
        }
        fNObject.setVelocities(velocitiesArray);
        fNObject.setThickness(0.05f);
        fNObject.setInverseMass(1.0f);
        fNObject.setBounce(0.0f);
        fNObject.setFriction(0.1f);
        fNObject.setDamping(0.0f);
        fNObject.setBendResistance(0.0f);
        fNObject.setMaxIterations(100);        
        fNObject.setMaxSelfCollisionIterations(100);
        fNObject.setStretchAndCompressionResistance(20.0f,10.0f);
        fNObject.setSelfCollisionFlags(false);
        fNObject.setCollisionFlags(true);        	    

        
        MFnNObjectData outputData;
        MObject mayaNObjectData = outputData.create();
        outputData.setObject(mayaNObjectData);
        
        outputData.setObjectPtr(&fNObject);        
        outputData.setCached(false);

        MDataHandle startStateOutputHandle = data.outputValue(startState);
        startStateOutputHandle.set(outputData.object());

        cerr<<"pull on startState\n";
	}
	else {
		stat = MS::kUnknownParameter;
	}
	return stat;
}



MStatus testNobjectNode::initialize()
{
	MStatus stat;

	MFnTypedAttribute tAttr;

	nextState = tAttr.create("nextState", "nxs", MFnData::kNObject, MObject::kNullObj, &stat );
	statCheck(stat, "failed to create nextState");
	tAttr.setWritable(true);
	tAttr.setStorable(true);
	tAttr.setHidden(true);

	inputGeom = tAttr.create("inputGeom", "ing", MFnData::kMesh, MObject::kNullObj, &stat );
	statCheck(stat, "failed to create inputGeom");
	tAttr.setWritable(true);
	tAttr.setStorable(true);
	tAttr.setHidden(true);

	currentState = tAttr.create("currentState", "cus", MFnData::kNObject, MObject::kNullObj, &stat );
	statCheck(stat, "failed to create currentState");
	tAttr.setWritable(true);
	tAttr.setStorable(false);
	tAttr.setHidden(true);

	startState = tAttr.create( "startState", "sts", MFnData::kNObject, MObject::kNullObj, &stat );
	statCheck(stat, "failed to create startState");
	tAttr.setWritable(true);
	tAttr.setStorable(false);
	tAttr.setHidden(true);

	outputGeom = tAttr.create("outputGeom", "outg", MFnData::kMesh, MObject::kNullObj, &stat );
	statCheck(stat, "failed to create outputGeom");
	tAttr.setWritable(true);
	tAttr.setStorable(false);
	tAttr.setHidden(true);

	MFnUnitAttribute uniAttr;
	currentTime = uniAttr.create( "currentTime", "ctm" , MFnUnitAttribute::kTime,  0.0, &stat  );    

	addAttribute(inputGeom);
	addAttribute(outputGeom);
	addAttribute(currentTime);
	addAttribute(startState);
	addAttribute(currentState);
	addAttribute(nextState);
	
	attributeAffects(inputGeom, outputGeom);
	attributeAffects(nextState, outputGeom);
	attributeAffects(inputGeom, startState);
	attributeAffects(currentTime, outputGeom);
	attributeAffects(currentTime, currentState);    
    attributeAffects(currentTime, startState);    

	return MStatus::kSuccess;
}

MStatus initializePlugin ( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin(obj, "Autodesk - nCloth Prototype 5", "8.5", "Any");

	status = plugin.registerNode ( "testNobjectNode", testNobjectNode::id,testNobjectNode ::creator, testNobjectNode::initialize );

	if ( !status )
	{
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin ( MObject obj )
{
	MStatus	  status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode(testNobjectNode::id);
	if ( !status )
	{
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
