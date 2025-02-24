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

////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//
// Produces the dependency graph command "pointOnSubd".
//
// This node is a simple example of how to query a subdivision surface as
// an input to a dependency node. This node takes a subdivision surface
// and a parameter point on subdivision surface and outputs the position
// and the normal of the surface at that point.
//
// The MEL script "connectObjectToPointOnSubd.mel" that accompanies this plug-in
// contains detailed documentation on how to use the node and provides
// a demonstration. 
//
////////////////////////////////////////////////////////////////////////

#include "pointOnSubdNode.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnSubdNames.h>
#include <maya/MFnSubdData.h>
#include <maya/MDataHandle.h>
#include <maya/MDataBlock.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnSubd.h>
#include <maya/MGlobal.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MPlug.h>

MTypeId     pointOnSubd::id( 0x80019 );

#include <maya/MIOStream.h>
#define McheckErr(status,message)		\
	if( MStatus::kSuccess != stat ) {	\
		cerr << message << "\n";		\
		return stat;					\
	}

// attributes
// 
MObject pointOnSubd::aSubd;
MObject pointOnSubd::aFaceFirst;
MObject pointOnSubd::aFaceSecond;
MObject	pointOnSubd::aRelativeUV;
MObject pointOnSubd::aU;
MObject pointOnSubd::aV;

MObject pointOnSubd::aPoint;
MObject pointOnSubd::aPointX;
MObject pointOnSubd::aPointY;
MObject pointOnSubd::aPointZ;
MObject pointOnSubd::aNormal;
MObject pointOnSubd::aNormalX;
MObject pointOnSubd::aNormalY;
MObject pointOnSubd::aNormalZ;

pointOnSubd::pointOnSubd() {}
pointOnSubd::~pointOnSubd() {}

MStatus pointOnSubd::compute( const MPlug& plug, MDataBlock& data )
//
//	Description:
//		This method computes the value of the given output plug based
//		on the values of the input attributes.
//
//	Arguments:
//		plug - the plug to compute
//		data - object that provides access to the attributes for this node
//
{
	MStatus returnStatus;
 
	// Check which output attribute we have been asked to compute.  If this 
	// node doesn't know how to compute it, we must return 
	// MS::kUnknownParameter.
	// 
	if( (plug == aPoint) || (plug == aNormal) ||
		(plug == aPointX) || (plug == aNormalX) ||
		(plug == aPointY) || (plug == aNormalY) ||
		(plug == aPointZ) || (plug == aNormalZ) ) {

		// Get a handle to the input attribute that we will need for the
		// computation.  If the value is being supplied via a connection 
		// in the dependency graph, then this call will cause all upstream  
		// connections to be evaluated so that the correct value is supplied.
		// 
		do {
			MDataHandle subdHandle = data.inputValue( aSubd, &returnStatus );
			if( returnStatus != MS::kSuccess ) {
				MGlobal::displayError( "ERROR: cannot get subd\n" );
				break;
			}
			
			MDataHandle faceFirstHandle =
				data.inputValue( aFaceFirst, &returnStatus );
			if( returnStatus != MS::kSuccess ) {
				MGlobal::displayError( "ERROR: cannot get face first\n" );
				break;
			}
			
			MDataHandle faceSecondHandle =
				data.inputValue( aFaceSecond, &returnStatus );
			if( returnStatus != MS::kSuccess ) {
				MGlobal::displayError( "ERROR: cannot get face2\n" );
				break;
			}
			
			MDataHandle uHandle = data.inputValue( aU, &returnStatus );
			if( returnStatus != MS::kSuccess ) {
				MGlobal::displayError( "ERROR: cannot get u\n" );
				break;
			}
			
			MDataHandle vHandle = data.inputValue( aV, &returnStatus );
			if( returnStatus != MS::kSuccess ) {
				MGlobal::displayError( "ERROR: cannot get v\n" );
				break;
			}

			MDataHandle relHandle = data.inputValue( aRelativeUV, &returnStatus );
			if( returnStatus != MS::kSuccess ) {
				MGlobal::displayError( "ERROR: cannot get relative UV\n" );
				break;
			}
			
			// Read the input value from the handle.
			//
			MStatus stat;
			MObject subdValue = subdHandle.asSubdSurface();
			MFnSubd subdFn( subdValue, &stat );
			McheckErr(stat,"ERROR creating subd function set"); 

			int faceFirstValue = faceFirstHandle.asInt();
			int faceSecondValue = faceSecondHandle.asInt();
			double uValue = uHandle.asDouble();
			double vValue = vHandle.asDouble();
			bool relUV = relHandle.asBool();

			MPoint point;
			MVector normal;

			MUint64 polyId;
			stat = MFnSubdNames::fromSelectionIndices( polyId, faceFirstValue,
													   faceSecondValue );
			McheckErr(stat,"ERROR converting indices"); 


			stat = subdFn.evaluatePositionAndNormal( polyId, uValue, vValue,
													 relUV, point, normal );
			normal.normalize();
			McheckErr(stat,"ERROR evaluating the position and the normal"); 

			// Get handles to the output attributes.  This is similar to the
			// "inputValue" call above except that no dependency graph 
			// computation will be done as a result of this call.
			// 
			MDataHandle pointHandle = data.outputValue( aPoint );
			pointHandle.set( point.x, point.y, point.z );
			data.setClean(plug);

			MDataHandle normalHandle = data.outputValue( aNormal );
			normalHandle.set( normal.x, normal.y, normal.z );
			data.setClean(plug);

		} while( false );
	}
	else {
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

void* pointOnSubd::creator()
//
//	Description:
//		this method exists to give Maya a way to create new objects
//      of this type. 
//
//	Return Value:
//		a new object of this type
//
{
	return new pointOnSubd;
}

MStatus pointOnSubd::initialize()
//
//	Description:
//		This method is called to create and initialize all of the attributes
//      and attribute dependencies for this node type.  This is only called 
//		once when the node type is registered with Maya.
//
//	Return Values:
//		MS::kSuccess
//		MS::kFailure
//		
{
	MStatus stat;

	MFnTypedAttribute subdAttr;
	aSubd = subdAttr.create( "subd", "s", MFnSubdData::kSubdSurface, MObject::kNullObj, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aSubd" );
 	subdAttr.setStorable(true);
 	subdAttr.setKeyable(false);
	subdAttr.setReadable( true );
	subdAttr.setWritable( true );
	subdAttr.setCached( false );
	stat = addAttribute( pointOnSubd::aSubd );
	McheckErr( stat, "cannot add pointOnSubd::aSubd" );

	MFnNumericAttribute faceFirstAttr;
	aFaceFirst = faceFirstAttr.create( "faceFirst", "ff",
									   MFnNumericData::kLong, 0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aFaceFirst" );
 	faceFirstAttr.setStorable(true);
 	faceFirstAttr.setKeyable(true);
	faceFirstAttr.setSoftMin( 0.0 );
	faceFirstAttr.setReadable( true );
	faceFirstAttr.setWritable( true );
	faceFirstAttr.setCached( false );
	stat = addAttribute( pointOnSubd::aFaceFirst );
	McheckErr( stat, "cannot add pointOnSubd::aFaceFirst" );

	MFnNumericAttribute faceSecondAttr;
	aFaceSecond = faceSecondAttr.create( "faceSecond", "fs",
										 MFnNumericData::kLong, 0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aFaceSecond" );
 	faceSecondAttr.setStorable(true);
 	faceSecondAttr.setKeyable(true);
	faceSecondAttr.setSoftMin( 0.0 );
	faceSecondAttr.setReadable( true );
	faceSecondAttr.setWritable( true );
	faceSecondAttr.setCached( false );
	stat = addAttribute( pointOnSubd::aFaceSecond );
	McheckErr( stat, "cannot add pointOnSubd::aFaceSecond" );

	MFnNumericAttribute uAttr;
	aU = uAttr.create( "uValue", "u", MFnNumericData::kDouble,
					   0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aU" );
 	uAttr.setStorable(true);
 	uAttr.setKeyable(true);
	uAttr.setSoftMin( 0.0 );
	uAttr.setSoftMax( 1.0 );
	uAttr.setReadable( true );
	uAttr.setWritable( true );
	uAttr.setCached( false );
	stat = addAttribute( aU );
	McheckErr( stat, "cannot add pointOnSubd::aU" );

	MFnNumericAttribute vAttr;
	aV = vAttr.create( "vValue", "v", MFnNumericData::kDouble,
					   0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aV" );
 	vAttr.setStorable(true);
 	vAttr.setKeyable(true);
	vAttr.setSoftMin( 0.0 );
	vAttr.setSoftMax( 1.0 );
	vAttr.setReadable( true );
	vAttr.setWritable( true );
	vAttr.setCached( false );
	stat = addAttribute( aV );
	McheckErr( stat, "cannot add pointOnSubd::aV" );

	MFnNumericAttribute relAttr;
	aRelativeUV = relAttr.create( "relative", "rel", MFnNumericData::kBoolean,
								  0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aRelativeUV" );
 	relAttr.setStorable(true);
 	relAttr.setKeyable(true);
	relAttr.setSoftMin( 0.0 );
	relAttr.setSoftMax( 1.0 );
	relAttr.setReadable( true );
	relAttr.setWritable( true );
	relAttr.setCached( false );
	stat = addAttribute( pointOnSubd::aRelativeUV );
	McheckErr( stat, "cannot add pointOnSubd::aRelativeUV" );

	MFnNumericAttribute pointXAttr;
	aPointX = pointXAttr.create( "pointX", "px", MFnNumericData::kDouble,
								0.0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aPointX" );
	pointXAttr.setWritable(false);
	pointXAttr.setStorable(false);
	pointXAttr.setReadable( true );
	pointXAttr.setCached( true );
	stat = addAttribute( aPointX );
	McheckErr( stat, "cannot add pointOnSubd::aPointX" );

	MFnNumericAttribute pointYAttr;
	aPointY = pointYAttr.create( "pointY", "py", MFnNumericData::kDouble,
								0.0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aPointY" );
	pointYAttr.setWritable(false);
	pointYAttr.setStorable(false);
	pointYAttr.setReadable( true );
	pointYAttr.setCached( true );
	stat = addAttribute( aPointY );
	McheckErr( stat, "cannot add pointOnSubd::aPointY" );

	MFnNumericAttribute pointZAttr;
	aPointZ = pointZAttr.create( "pointZ", "pz", MFnNumericData::kDouble,
								0.0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aPointZ" );
	pointZAttr.setWritable(false);
	pointZAttr.setStorable(false);
	pointZAttr.setReadable( true );
	pointZAttr.setCached( true );
	stat = addAttribute( aPointZ );
	McheckErr( stat, "cannot add pointOnSubd::aPointZ" );

	MFnNumericAttribute pointAttr;
	aPoint = pointAttr.create( "point", "p", aPointX, aPointY, aPointZ, &stat);
	McheckErr( stat, "cannot create pointOnSubd::aPoint" );
	pointAttr.setWritable(false);
	pointAttr.setStorable(false);
	pointAttr.setReadable( true );
	pointAttr.setCached( true );
	stat = addAttribute( aPoint );
	McheckErr( stat, "cannot add pointOnSubd::aPoint" );

	MFnNumericAttribute normalXAttr;
	aNormalX = normalXAttr.create( "normalX", "nx", MFnNumericData::kDouble,
								   0.0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aNormal" );
	normalXAttr.setWritable(false);
	normalXAttr.setStorable(false);
	normalXAttr.setReadable( true );
	normalXAttr.setCached( true );
	stat = addAttribute( aNormalX );
	McheckErr( stat, "cannot add pointOnSubd::aNormalX" );

	MFnNumericAttribute normalYAttr;
	aNormalY = normalYAttr.create( "normalY", "ny", MFnNumericData::kDouble,
								   0.0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aNormal" );
	normalYAttr.setWritable(false);
	normalYAttr.setStorable(false);
	normalYAttr.setReadable( true );
	normalYAttr.setCached( true );
	stat = addAttribute( aNormalY );
	McheckErr( stat, "cannot add pointOnSubd::aNormalY" );

	MFnNumericAttribute normalZAttr;
	aNormalZ = normalZAttr.create( "normalZ", "nz", MFnNumericData::kDouble,
								   0.0, &stat );
	McheckErr( stat, "cannot create pointOnSubd::aNormal" );
	normalZAttr.setWritable(false);
	normalZAttr.setStorable(false);
	normalZAttr.setReadable( true );
	normalZAttr.setCached( true );
	stat = addAttribute( aNormalZ );
	McheckErr( stat, "cannot add pointOnSubd::aNormalZ" );

	MFnNumericAttribute normalAttr;
	aNormal = normalAttr.create("normal","n",aNormalX,aNormalY,aNormalZ,&stat);
	McheckErr( stat, "cannot create pointOnSubd::aNormal" );
	normalAttr.setWritable(false);
	normalAttr.setStorable(false);
	normalAttr.setReadable( true );
	normalAttr.setCached( true );
	stat = addAttribute( aNormal );
	McheckErr( stat, "cannot add pointOnSubd::aNormal" );


	// Set up a dependency between the input and the output.  This will cause
	// the output to be marked dirty when the input changes.  The output will
	// then be recomputed the next time the value of the output is requested.
	//
	stat = attributeAffects( aSubd, aPoint );
	stat = attributeAffects( aSubd, aPointX );
	stat = attributeAffects( aSubd, aPointY );
	stat = attributeAffects( aSubd, aPointZ );
	stat = attributeAffects( aSubd, aNormal );
	stat = attributeAffects( aSubd, aNormalX );
	stat = attributeAffects( aSubd, aNormalY );
	stat = attributeAffects( aSubd, aNormalZ );

	stat = attributeAffects( aFaceFirst, aPoint );
	stat = attributeAffects( aFaceFirst, aPointX );
	stat = attributeAffects( aFaceFirst, aPointY );
	stat = attributeAffects( aFaceFirst, aPointZ );
	stat = attributeAffects( aFaceFirst, aNormal );
	stat = attributeAffects( aFaceFirst, aNormalX );
	stat = attributeAffects( aFaceFirst, aNormalY );
	stat = attributeAffects( aFaceFirst, aNormalZ );

	stat = attributeAffects( aFaceSecond, aPoint );
	stat = attributeAffects( aFaceSecond, aPointX );
	stat = attributeAffects( aFaceSecond, aPointY );
	stat = attributeAffects( aFaceSecond, aPointZ );
	stat = attributeAffects( aFaceSecond, aNormal );
	stat = attributeAffects( aFaceSecond, aNormalX );
	stat = attributeAffects( aFaceSecond, aNormalY );
	stat = attributeAffects( aFaceSecond, aNormalZ );

	stat = attributeAffects( aU, aPoint );
	stat = attributeAffects( aU, aPointX );
	stat = attributeAffects( aU, aPointY );
	stat = attributeAffects( aU, aPointZ );
	stat = attributeAffects( aU, aNormal );
	stat = attributeAffects( aU, aNormalX );
	stat = attributeAffects( aU, aNormalY );
	stat = attributeAffects( aU, aNormalZ );

	stat = attributeAffects( aV, aPoint );
	stat = attributeAffects( aV, aPointX );
	stat = attributeAffects( aV, aPointY );
	stat = attributeAffects( aV, aPointZ );
	stat = attributeAffects( aV, aNormal );
	stat = attributeAffects( aV, aNormalX );
	stat = attributeAffects( aV, aNormalY );
	stat = attributeAffects( aV, aNormalZ );

	stat = attributeAffects( aRelativeUV, aPoint );
	stat = attributeAffects( aRelativeUV, aPointX );
	stat = attributeAffects( aRelativeUV, aPointY );
	stat = attributeAffects( aRelativeUV, aPointZ );
	stat = attributeAffects( aRelativeUV, aNormal );
	stat = attributeAffects( aRelativeUV, aNormalX );
	stat = attributeAffects( aRelativeUV, aNormalY );
	stat = attributeAffects( aRelativeUV, aNormalZ );

	return MS::kSuccess;

}

// ---------------------------------------------------------------

MStatus initializePlugin( MObject obj )
//
//	Description:
//		this method is called when the plug-in is loaded into Maya.  It 
//		registers all of the services that this plug-in provides with 
//		Maya.
//
//	Arguments:
//		obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode( "pointOnSubd",
								  pointOnSubd::id,
								  pointOnSubd::creator,
								  pointOnSubd::initialize );
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
//
//	Description:
//		this method is called when the plug-in is unloaded from Maya. It 
//		deregisters all of the services that it was providing.
//
//	Arguments:
//		obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( pointOnSubd::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
