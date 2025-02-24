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
// Produces the dependency graph node "fullLoft".
//
// This plug-in demonstrates how to take an array of NURBS curves as input and produce a NURBS surface as output.
// The input curves must have the same number of knots and both form and degree are ignored.
//
// To use this command:
//	(1) Draw two or more curves.
//	(2) Select the curves and execute the MEL command "source fullLoftNode.mel".
//		This creates the fullLoft node and an output surface as well as several rebuildCurve nodes to
//		provide a uniform number of CVs for the fullLoft node. 
//
// This is a simplified version of the internal loft node in Maya.
//
////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <maya/MIOStream.h>
#include <math.h>

#include <maya/MPxNode.h>
#include <maya/MPxCommand.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnPlugin.h>

#include <maya/MFnNurbsCurve.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnNurbsCurveData.h>
#include <maya/MFnNurbsSurfaceData.h>

#include <maya/MPointArray.h>
#include <maya/MDoubleArray.h>

#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <maya/MGlobal.h>
#include <maya/MDagPath.h>

class fullLoft : public MPxNode
{
public:
						fullLoft() {};
					~fullLoft() override;

	MStatus		compute( const MPlug& plug, MDataBlock& data ) override;
	MObject				loft( MArrayDataHandle &inputArray, MObject &surfFn, 
							  MStatus &stat );
	static	void*		creator();
	static	MStatus		initialize();

public:
	static	MObject     inputCurve;		// The input curve.
	static	MObject		outputSurface;	// The output surface
	static	MTypeId		id;				// The IFF type id
};


MTypeId	    fullLoft::id( 0x80008 );
MObject     fullLoft::inputCurve;
MObject	    fullLoft::outputSurface;


// Error macro: if not successful, print error message and return failure.
// Assumes that "stat" contains the error value
// Note that if (!stat) is the most efficient way to test for failure
#define PERRORfail(s) \
if (!stat) { \
    stat.perror(s); \
	return stat; \
}

// Error macro: if not successful, print error message and return a NULL object
// Assumes that "stat" contains the error value
#define PERRORnull(s) \
if (!stat) { \
    stat.perror(s); \
	return MObject::kNullObj; \
}

fullLoft::~fullLoft ()
{
}

void* fullLoft::creator()
{
	return new fullLoft();
}

MStatus fullLoft::initialize()
{
	MStatus stat;
	MFnTypedAttribute typedAttr;
	
	inputCurve=typedAttr.create( "inputCurve", "in",
										 MFnNurbsCurveData::kNurbsCurve,
										 MObject::kNullObj,
										 &stat );
	PERRORfail("initialize create input attribute");
	stat = typedAttr.setArray( true );
	PERRORfail("initialize set input attribute array");
	
	outputSurface=typedAttr.create( "outputSurface", "out",
											MFnNurbsSurfaceData::kNurbsSurface,
											MObject::kNullObj,
											&stat );
	PERRORfail("initialize create output attribute");
	stat = typedAttr.setStorable( false );
	PERRORfail("initialize set output attribute storable");

	stat = addAttribute( inputCurve );
	PERRORfail("addAttribute(inputCurve)");

	stat = addAttribute( outputSurface );
	PERRORfail("addAttribute(outputSurface)");

	stat = attributeAffects( inputCurve, outputSurface );
	PERRORfail("attributeAffects(inputCurve, outputSurface)");

	return MS::kSuccess;
}

MObject fullLoft::loft( MArrayDataHandle &inputArray, MObject &newSurfData,
					  MStatus &stat )
{
	MFnNurbsSurface surfFn;
	MPointArray cvs;
	MDoubleArray ku, kv;
	int i, j;
	int numCVs;
	int numCurves = inputArray.elementCount ();

	// Ensure that we have at least 1 element in the input array
	// We must not do an inputValue on an element that does not
	// exist.
	if ( numCurves < 1 )
		return MObject::kNullObj;

	// Count the number of CVs
	inputArray.jumpToElement(0);
	MDataHandle elementHandle = inputArray.inputValue(&stat);
	if (!stat) {
		stat.perror("fullLoft::loft: inputValue");
		return MObject::kNullObj;
	}
	MObject countCurve (elementHandle.asNurbsCurve());
	MFnNurbsCurve countCurveFn (countCurve);
	numCVs = countCurveFn.numCVs (&stat);
	PERRORnull("fullLoft::loft counting CVs");

	// Create knot vectors for U and V
	// U dimension contains one CV from each curve, triple knotted
	for (i = 0; i < numCurves; i++)
	{
		ku.append (double (i));
		ku.append (double (i));
		ku.append (double (i));
	}

	// V dimension contains all of the CVs from one curve, triple knotted at
	// the ends
	kv.append( 0.0 );
	kv.append( 0.0 );
	kv.append( 0.0 );

	for ( i = 1; i < numCVs - 3; i ++ )
		kv.append( (double) i );

	kv.append( numCVs-3 );
	kv.append( numCVs-3 );
	kv.append( numCVs-3 );

	// Build the surface's CV array
	for (int curveNum = 0; curveNum < numCurves; curveNum++)
	{
		MObject curve (inputArray.inputValue ().asNurbsCurve ());
		MFnNurbsCurve curveFn (curve);
		MPointArray curveCVs;

		stat = curveFn.getCVs (curveCVs, MSpace::kWorld);
		PERRORnull("fullLoft::loft getting CVs");

		if (curveCVs.length() != (unsigned)numCVs)
			stat = MS::kFailure;
		PERRORnull("fullLoft::loft inconsistent number of CVs - rebuild curves");

		// Triple knot for every curve but the first
		int repeats = (curveNum == 0) ? 1 : 3;

		for (j = 0; j < repeats; j++)
			for ( i = 0; i < numCVs; i++ )
				cvs.append (curveCVs [i]);

		stat = inputArray.next ();
	}
	MObject surf = surfFn.create(cvs, ku, kv, 3, 3,
								 MFnNurbsSurface::kOpen,
								 MFnNurbsSurface::kOpen,
								 false, newSurfData, &stat );
	PERRORnull ("fullLoft::Loft create surface");

	return surf;
}

MStatus fullLoft::compute( const MPlug& plug, MDataBlock& data )
{
	MStatus stat;

	if ( plug == outputSurface )	// loft inputCurves into surface
	{
		MArrayDataHandle inputArrayData = data.inputArrayValue( inputCurve,
																&stat );
		PERRORfail("fullLoft::compute getting input array data");
		MDataHandle surfHandle = data.outputValue( fullLoft::outputSurface );
		PERRORfail("fullLoft::compute getting output data handle");
		
		MFnNurbsSurfaceData dataCreator;
		MObject newSurfData = dataCreator.create( &stat );
		PERRORfail("fullLoft::compute creating new nurbs surface data block");
		
		/* MObject newSurf = */ loft(inputArrayData, newSurfData,  stat );
		// No error message is needed - fullLoft::loft will output one
		if (!stat)
			return stat;
		
		// newSurf is the new surface object, but it has been packed
		// into the datablock we created for it, and the data block
		// is what we must put onto the plug.
		stat = surfHandle.set( newSurfData );
		PERRORfail("fullLoft::compute setting surface handle");
		
		stat = data.setClean( plug );
		PERRORfail("fullLoft::compute cleaning outputSurface plug");
	}
	else
	{
		return MS::kUnknownParameter;
	}
	
	return MS::kSuccess;
}

MStatus initializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode( "fullLoft", fullLoft::id, fullLoft::creator,
						 fullLoft::initialize );
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( fullLoft::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}
	
	return status;
}
