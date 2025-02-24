//-
// ==========================================================================
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
// 
// This plug-in implements a dynExprField node for a uniform field.
// This allows the per particle attributes to drive the field's attributes. 
//
// To run the plug-in, do the following:
// 
// source dynExprFieldTest.mel
// dynExprFieldTest;
//
////////////////////////////////////////////////////////////////////////

#include <math.h>

#include "dynExprField.h"

#include <maya/MIOStream.h>
#include <maya/MTime.h>
#include <maya/MFloatVector.h>
#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MString.h>
#include <maya/MMatrix.h>
#include <maya/MArrayDataBuilder.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnArrayAttrsData.h>

MObject dynExprField::mDirection;

MTypeId dynExprField::id( 0x00107340 );

void *dynExprField::creator()
{
    return new dynExprField;
}

MStatus dynExprField::initialize()
//
//	Descriptions:
//		Initialize the node, attributes.
//
{
	MStatus status;
	MFnNumericAttribute numAttr;

	// create the field basic attributes.
	//
	mDirection = numAttr.createPoint("direction","dir");
	numAttr.setDefault( 0.0, 1.0, 0.0 );
	numAttr.setKeyable(true);
	numAttr.setStorable(true);
	numAttr.setReadable(true);
	numAttr.setWritable(true);
	status = addAttribute( mDirection );
	attributeAffects(mDirection, mOutputForce);

	return( MS::kSuccess );
}


MStatus dynExprField::compute(const MPlug& plug, MDataBlock& block)
//
//	Descriptions:
//		compute output force.
//
{
	MStatus status;

	if( !(plug == mOutputForce) )
        return( MS::kUnknownParameter );

	// get the logical index of the element this plug refers to.
	//
	int multiIndex = plug.logicalIndex( &status );
	McheckErr(status, "ERROR in plug.logicalIndex.\n");

	// Get input data handle, use outputArrayValue since we do not
	// want to evaluate both inputs, only the one related to the
	// requested multiIndex. Evaluating both inputs at once would cause
	// a dependency graph loop.
	
	MArrayDataHandle hInputArray = block.outputArrayValue( mInputData, &status );
	McheckErr(status,"ERROR in hInputArray = block.outputArrayValue().\n");
	
	status = hInputArray.jumpToElement( multiIndex );
	McheckErr(status, "ERROR: hInputArray.jumpToElement failed.\n");
	
	// get children of aInputData.
	
	MDataHandle hCompond = hInputArray.inputValue( &status );
	McheckErr(status, "ERROR in hCompond=hInputArray.inputValue\n");
	
	MDataHandle hPosition = hCompond.child( mInputPositions );
	MObject dPosition = hPosition.data();
	MFnVectorArrayData fnPosition( dPosition );
	MVectorArray points = fnPosition.array( &status );
	McheckErr(status, "ERROR in fnPosition.array(), not find points.\n");
	
	// Comment out the following since velocity, and mass are 
	// not needed in this field.
	//
	// MDataHandle hVelocity = hCompond.child( mInputVelocities );
	// MObject dVelocity = hVelocity.data();
	// MFnVectorArrayData fnVelocity( dVelocity );
	// MVectorArray velocities = fnVelocity.array( &status );
	// McheckErr(status, "ERROR in fnVelocity.array(), not find velocities.\n");
	//
	// MDataHandle hMass = hCompond.child( mInputMass );
	// MObject dMass = hMass.data();
	// MFnDoubleArrayData fnMass( dMass );
	// MDoubleArray masses = fnMass.array( &status );
	// McheckErr(status, "ERROR in fnMass.array(), not find masses.\n");

	// The attribute mInputPPData contains the attribute in an array form 
	// parpared by the particleShape if the particleShape has per particle 
	// attribute fieldName_attrName.  
	//
	// Suppose a field with the name dynExprField1 is connecting to 
	// particleShape1, and the particleShape1 has per particle float attribute
	// dynExprField1_magnitude and vector attribute dynExprField1_direction,
	// then hInputPPArray will contains a MdoubleArray with the corresponding
	// name "magnitude" and a MvectorArray with the name "direction".  This 
	// is a mechanism to allow the field attributes being driven by dynamic 
	// expression.
	MArrayDataHandle mhInputPPData = block.inputArrayValue( mInputPPData, &status );
	McheckErr(status,"ERROR in mhInputPPData = block.inputArrayValue().\n");

	status = mhInputPPData.jumpToElement( multiIndex );
	McheckErr(status, "ERROR: mhInputPPArray.jumpToElement failed.\n");

	MDataHandle hInputPPData = mhInputPPData.inputValue( &status );
	McheckErr(status, "ERROR in hInputPPData = mhInputPPData.inputValue\n");

	MObject dInputPPData = hInputPPData.data();
	MFnArrayAttrsData inputPPArray( dInputPPData );

	MDataHandle hOwnerPPData = block.inputValue( mOwnerPPData, &status );
	McheckErr(status, "ERROR in hOwnerPPData = block.inputValue\n");

	MObject dOwnerPPData = hOwnerPPData.data();
	MFnArrayAttrsData ownerPPArray( dOwnerPPData );

	const MString magString("magnitude");
	MFnArrayAttrsData::Type doubleType(MFnArrayAttrsData::kDoubleArray);

	bool arrayExist;
	MDoubleArray magnitudeArray;
	arrayExist = inputPPArray.checkArrayExist(magString, doubleType, &status);
	// McheckErr(status, "ERROR in checkArrayExist(magnitude)\n");
	if(arrayExist) {
	    magnitudeArray = inputPPArray.getDoubleData(magString, &status);
	    // McheckErr(status, "ERROR in inputPPArray.doubleArray(magnitude)\n");
	}

	MDoubleArray magnitudeOwnerArray;
	arrayExist = ownerPPArray.checkArrayExist(magString, doubleType, &status);
	// McheckErr(status, "ERROR in checkArrayExist(magnitude)\n");
	if(arrayExist) {
	    magnitudeOwnerArray = ownerPPArray.getDoubleData(magString, &status);
	    // McheckErr(status, "ERROR in ownerPPArray.doubleArray(magnitude)\n");
	}

	const MString dirString("direction");
	MFnArrayAttrsData::Type vectorType(MFnArrayAttrsData::kVectorArray);

	arrayExist = inputPPArray.checkArrayExist(dirString, vectorType, &status);
        MVectorArray directionArray;
	// McheckErr(status, "ERROR in checkArrayExist(direction)\n");
	if(arrayExist) {
	    directionArray = inputPPArray.getVectorData(dirString, &status);
	    // McheckErr(status, "ERROR in inputPPArray.vectorArray(direction)\n");
	}

	arrayExist = ownerPPArray.checkArrayExist(dirString, vectorType, &status);
        MVectorArray directionOwnerArray;
	// McheckErr(status, "ERROR in checkArrayExist(direction)\n");
	if(arrayExist) {
	    directionOwnerArray = ownerPPArray.getVectorData(dirString, &status);
	    // McheckErr(status, "ERROR in ownerPPArray.vectorArray(direction)\n");
	}

	// Compute the output force.
	//
	MVectorArray forceArray;

	apply( block, points.length(), magnitudeArray, magnitudeOwnerArray, 
	       directionArray, directionOwnerArray, forceArray );

	// get output data handle
	//
	MArrayDataHandle hOutArray = block.outputArrayValue( mOutputForce, &status);
	McheckErr(status, "ERROR in hOutArray = block.outputArrayValue.\n");
	MArrayDataBuilder bOutArray = hOutArray.builder( &status );
	McheckErr(status, "ERROR in bOutArray = hOutArray.builder.\n");

	// get output force array from block.
	//
	MDataHandle hOut = bOutArray.addElement(multiIndex, &status);
	McheckErr(status, "ERROR in hOut = bOutArray.addElement.\n");

	MFnVectorArrayData fnOutputForce;
	MObject dOutputForce = fnOutputForce.create( forceArray, &status );
	McheckErr(status, "ERROR in dOutputForce = fnOutputForce.create\n");

	// update data block with new output force data.
	//
	hOut.set( dOutputForce );
	block.setClean( plug );

	return( MS::kSuccess );
}

MStatus dynExprField::iconSizeAndOrigin(GLuint& width,
					GLuint& height,
					GLuint& xbo,
					GLuint& ybo   )
//
//	This method is not required to be overridden.  It should be overridden
//	if the plug-in has custom icon.
//
//	The width and height have to be a multiple of 32 on Windows and 16 on 
//	other platform.
//
//	Define an 8x8 icon at the lower left corner of the 32x32 grid. 
//	(xbo, ybo) of (4,4) makes sure the icon is center at origin.
//
{
	width = 32;
	height = 32;
	xbo = 4;
	ybo = 4;
	return MS::kSuccess;
}

MStatus dynExprField::iconBitmap(GLubyte* bitmap)
//
//	This method is not required to be overridden.  It should be overridden
//	if the plug-in has custom icon.
//
//	Define an 8x8 icon at the lower left corner of the 32x32 grid. 
//	(xbo, ybo) of (4,4) makes sure the icon is center at origin.
{
	bitmap[0]  = 0x18;
	bitmap[4]  = 0x18;
	bitmap[8]  = 0x18;
	bitmap[12] = 0x18;
	bitmap[16] = 0x18;
	bitmap[20] = 0x5A;
	bitmap[24] = 0x3C;
	bitmap[28] = 0x18;

	return MS::kSuccess;
}

MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "6.0", "Any");

	status = plugin.registerNode(  "dynExprField", 
				       dynExprField::id,
				      &dynExprField::creator, 
				      &dynExprField::initialize,
				       MPxNode::kFieldNode );
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterNode( dynExprField::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}

double dynExprField::magnitude( MDataBlock& block )
{
	MStatus status;
	MDataHandle hValue = block.inputValue( mMagnitude, &status );

	double value = 0.0;
	if( status == MS::kSuccess )
		value = hValue.asDouble();

	return( value );
}

MVector dynExprField::direction( MDataBlock& block )
{
	MFloatVector &fV = block.inputValue(mDirection).asFloatVector();
	return( MVector(fV.x, fV.y, fV.z) );
}

void dynExprField::apply(
MDataBlock         &block,
int                 receptorSize,
const MDoubleArray &magnitudeArray,
const MDoubleArray &magnitudeOwnerArray,
const MVectorArray &directionArray,
const MVectorArray &directionOwnerArray,
MVectorArray       &outputForce
)
//
//      Compute output force for each particle.  If there exists the 
//      corresponding per particle attribute, use the data passed from
//      particle shape (stored in magnitudeArray and directionArray).  
//      Otherwise, use the attribute value from the field.
//
{
        // get the default values
	MVector defaultDir = direction(block);
	double  defaultMag = magnitude(block);
	int magArraySize = magnitudeArray.length();
	int dirArraySize = directionArray.length();
	int magOwnerArraySize = magnitudeOwnerArray.length();
	int dirOwnerArraySize = directionOwnerArray.length();
	int numOfOwner = magOwnerArraySize;
	if( dirOwnerArraySize > numOfOwner )
	    numOfOwner = dirOwnerArraySize;

	double  magnitude = defaultMag;
	MVector direction = defaultDir;

	for (int ptIndex = 0; ptIndex < receptorSize; ptIndex ++ ) {
	    if(receptorSize == magArraySize)
	        magnitude = magnitudeArray[ptIndex];
	    if(receptorSize == dirArraySize)
	        direction = directionArray[ptIndex];
	    if( numOfOwner > 0) {
	        for( int nthOwner = 0; nthOwner < numOfOwner; nthOwner++ ) {
		    if(magOwnerArraySize == numOfOwner)
		        magnitude = magnitudeOwnerArray[nthOwner];
		    if(dirOwnerArraySize == numOfOwner)
		        direction = directionOwnerArray[nthOwner];
		    outputForce.append( direction * magnitude );
		}
	    } else {
	        outputForce.append( direction * magnitude );
	    }
	}
}
