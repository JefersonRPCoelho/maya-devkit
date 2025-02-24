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

#include "testNucleusNode.h"
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
#include <math.h>
#include "stdio.h"

/*

This example show a custom solver at work.  Two nCloth objects are created,
one is disconnected from the default nucleus solver, and hooked to
this custom solver node, which will just compute a sine wave motion on
the cloth in time.

A custom solver needs to have a minimum of 3 attributes:

-startState     To be connected from the cloth object to the solver
-currentState   To be connected from the cloth object to the solver
-nextState      To be connected from the solver object to the cloth

and usually a 4th attribute that is the current time.

The idea is that when a solve is needed, the cloth object will pull on the
nextState attribute.  At this point the solver should pull on either the
currentState attribute or the startState, depending on the current time.
Once it has the state information, the solver can extract that information,
solve one step, and stuff that information back into the MnCloth to 
complete the solve.

Below is some example code to test this plugin:

//---------------------------------------------------------------------------------
// Note: Before running this code, make sure the plugin testNucleusNode is loaded!
//---------------------------------------------------------------------------------
global proc setupCustomSolverScene()
{

    file -f -new;

    string $pPlane1[] = `polyPlane -w 5 -h 5 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1`;
    move -r -10 0 0;
    createNCloth 0;

    string $pPlane2[] = `polyPlane -w 5 -h 5 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1`;
    createNCloth 0;

    //Hookup plane2 (the cloth object created for plane2 is named nClothShape2) to our custom solver instead.

    //First, disconnect it from the default nucleus solver:
    disconnectAttr nClothShape2.currentState nucleus1.inputActive[1];
    disconnectAttr nClothShape2.startState nucleus1.inputActiveStart[1];
    disconnectAttr nucleus1.outputObjects[1] nClothShape2.nextState;

    //create our custom solver:
    createNode testNucleusNode;

    //Hookup plane2 to our custom solver:
    connectAttr testNucleusNode1.nextState[0] nClothShape2.nextState;
    connectAttr nClothShape2.currentState testNucleusNode1.currentState[0];
    connectAttr nClothShape2.startState testNucleusNode1.startState[0];
    connectAttr time1.outTime testNucleusNode1.currentTime;

}

*/

const MTypeId testNucleusNode::id( 0x85002 );


#include <maya/MFnNObjectData.h>
#include <maya/MnCloth.h>

MObject testNucleusNode::startState;
MObject testNucleusNode::currentState;
MObject testNucleusNode::nextState;
MObject testNucleusNode::currentTime;


inline void statCheck( MStatus stat, MString msg )
{
	if ( !stat )
	{
		cout<<msg<<"\n";
	}
}

MStatus testNucleusNode::compute(const MPlug &plug, MDataBlock &data)
{
	MStatus stat;

	if ( plug == nextState )
	{
        
        //get the value of the currentTime 
        MTime currTime = data.inputValue(currentTime).asTime();        
        MObject inputData;
        //pull on start state or current state depending on the current time.
        if(currTime.value() <= 0.0) {
            MArrayDataHandle multiDataHandle = data.inputArrayValue(startState);
            multiDataHandle.jumpToElement(0);
            inputData =multiDataHandle.inputValue().data();
        }
        else {
            MArrayDataHandle multiDataHandle = data.inputArrayValue(currentState);
            multiDataHandle.jumpToElement(0);
            inputData =multiDataHandle.inputValue().data();
        }                
                
        MFnNObjectData inputNData(inputData);                
        MnCloth * nObj = NULL;
        inputNData.getObjectPtr(nObj);        
        
        MFloatPointArray points;
        nObj->getPositions(points);
        unsigned int ii;
        for(ii=0;ii<points.length();ii++) {            
            points[ii].y = (float) sin(points[ii].x + currTime.value()*4.0f*(3.1415f/180.0f));
        }
        nObj->setPositions(points);

        delete nObj;        
        data.setClean(plug);
	}
	else if ( plug == currentState )
	{        
	    data.setClean(plug);

	}
    else if (plug == startState) {        
	    data.setClean(plug);
    }
    else {
		stat = MS::kUnknownParameter;
	}
	return stat;
}



MStatus testNucleusNode::initialize()
{
	MStatus stat;

	MFnTypedAttribute tAttr;

	startState = tAttr.create("startState", "sst", MFnData::kNObject, MObject::kNullObj, &stat );

	statCheck(stat, "failed to create startState");
	tAttr.setWritable(true);
	tAttr.setStorable(true);
	tAttr.setHidden(true);
    tAttr.setArray(true);


    currentState = tAttr.create("currentState", "cst", MFnData::kNObject, MObject::kNullObj, &stat );

	statCheck(stat, "failed to create currentState");
	tAttr.setWritable(true);
	tAttr.setStorable(true);
	tAttr.setHidden(true);
    tAttr.setArray(true);
	

    nextState = tAttr.create("nextState", "nst", MFnData::kNObject, MObject::kNullObj, &stat );

	statCheck(stat, "failed to create nextState");
	tAttr.setWritable(true);
	tAttr.setStorable(true);
	tAttr.setHidden(true);
    tAttr.setArray(true);

   	MFnUnitAttribute uniAttr;
	currentTime = uniAttr.create( "currentTime", "ctm" , MFnUnitAttribute::kTime,  0.0, &stat  );    	

	addAttribute(startState);
	addAttribute(currentState);
	addAttribute(nextState);
    addAttribute(currentTime);
	
	attributeAffects(startState, nextState);
	attributeAffects(currentState, nextState);	
    attributeAffects(currentTime, nextState);	

	return MStatus::kSuccess;
}

MStatus initializePlugin ( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin(obj, "Autodesk - nCloth Prototype 4", "8.5", "Any");	

	status = plugin.registerNode ( "testNucleusNode", testNucleusNode::id,testNucleusNode ::creator, testNucleusNode::initialize );

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

	status = plugin.deregisterNode(testNucleusNode::id);
	if ( !status )
	{
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
