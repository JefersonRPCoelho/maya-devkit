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

// Example Plugin: circleNode.cpp
//
// This plug-in is an example of a user-defined dependency graph node.
// It takes a number as input (such as time) and generates two output
// numbers one which describes a sine curve as the input varies and
// one that generates a cosine curve. If these two are hooked up to
// the x and z translation attributes of an object the object will describe
// move through a circle in the xz plane as time is changed.
//
// Executing the command "source circleNode" will run the MEL script which will
// create a new "Circle" menu with a single item. Selecting this will build
// a simple model (a sphere which follows a circular path) which can be played back,
// by clicking on the "play" icon on the time slider.  Note: the circleNode
// plugin needs to be loaded before the "Circle" menu item can be executed
// properly.
//
// The node has two additional attributes which can be changed to affect
// the animation, "scale" which defines the size of the circular path, and
// "frames" which defines the number of frames required for a complete circuit
// of the path. Either of these can be hooked up to other nodes, or can
// be simply set via the MEL command "setAttr" operating on the circle node
// "circleNode1" created by the MEL script. For example, "setAttr circleNode1.scale #"
// will change the size of the circle and "setAttr circleNode1.frames #"
// will cause objects to complete a circle in indicated number of frames.


#include <string.h>
#include <maya/MIOStream.h>

#include <maya/MPxNode.h> 

#include <maya/MString.h> 
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MVector.h>

#include <maya/MFnPlugin.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>

#include <math.h>

// The circle class defines the attributes
// and methods necessary for the circleNode plugin
//
class circle : public MPxNode
{
public:
						circle();
					~circle() override; 

	MStatus		compute( const MPlug& plug, MDataBlock& data ) override;

	static	void*		creator();
	static	MStatus		initialize();
 
public:
	static	MObject		input;		// The input value.
	static	MObject		sOutput;	// The sinusoidal output value.
	static	MObject		cOutput;	// The cosinusoidal output value.
	static	MObject		frames;		// Number of frames for one circle.
	static	MObject		scale;		// Size of circle.
	static	MTypeId		id;
};

MTypeId     circle::id( 0x80005 );
MObject     circle::input;        
MObject     circle::sOutput;       
MObject     circle::cOutput;       
MObject	    circle::frames;
MObject 	circle::scale;

// The creator method creates an instance of the circleNode class
// and is the first method called by Maya
// when a circleNode needs to be created.
//
void* circle::creator()
{
	return new circle();
}

// The initialize routine is called after the node has been created.
// It sets up the input and output attributes and adds them to the node.
// Finally the dependencies are arranged so that when the inputs
// change Maya knowns to call compute to recalculate the output values.
// The inputs are: input, scale, frames
// The outputs are: sineOutput, cosineOutput
//
MStatus circle::initialize()
{
	MFnNumericAttribute nAttr;
	MStatus				stat;

	// Setup the input attributes
	//
	input = nAttr.create( "input", "in", MFnNumericData::kFloat, 0.0,
			&stat );
	CHECK_MSTATUS( stat );
 	CHECK_MSTATUS( nAttr.setStorable( true ) );

	scale = nAttr.create( "scale", "sc", MFnNumericData::kFloat, 10.0,
			&stat );
	CHECK_MSTATUS( stat );
	CHECK_MSTATUS( nAttr.setStorable( true ) );

	frames = nAttr.create( "frames", "fr", MFnNumericData::kFloat, 48.0,
			&stat );
	CHECK_MSTATUS( stat );
	CHECK_MSTATUS( nAttr.setStorable( true ) );

	// Setup the output attributes
	//
	sOutput = nAttr.create( "sineOutput", "so", MFnNumericData::kFloat,
			0.0, &stat );
	CHECK_MSTATUS( stat );
	CHECK_MSTATUS( nAttr.setWritable( false ) );
	CHECK_MSTATUS( nAttr.setStorable( false ) );

	cOutput = nAttr.create( "cosineOutput", "co", MFnNumericData::kFloat,
			0.0, &stat );
	CHECK_MSTATUS( stat );
	CHECK_MSTATUS( nAttr.setWritable( false ) );
	CHECK_MSTATUS( nAttr.setStorable( false ) );

	// Add the attributes to the node
	//
	CHECK_MSTATUS( addAttribute( input ) );
	CHECK_MSTATUS( addAttribute( scale ) );
	CHECK_MSTATUS( addAttribute( frames ) );
	CHECK_MSTATUS( addAttribute( sOutput ) );
	CHECK_MSTATUS( addAttribute( cOutput ) );

	// Set the attribute dependencies
	//
	CHECK_MSTATUS( attributeAffects( input, sOutput ) );
	CHECK_MSTATUS( attributeAffects( input, cOutput ) );
	CHECK_MSTATUS( attributeAffects( scale, sOutput ) );
	CHECK_MSTATUS( attributeAffects( scale, cOutput ) );
	CHECK_MSTATUS( attributeAffects( frames, sOutput ) );
	CHECK_MSTATUS( attributeAffects( frames, cOutput ) );

	return MS::kSuccess;
} 

// The constructor does nothing
//
circle::circle() {}

// The destructor does nothing
//
circle::~circle() {}

// The compute method is called by Maya when the input values
// change and the output values need to be recomputed.
// The input values are read then using sinf() and cosf()
// the output values are stored on the output plugs.
//
MStatus circle::compute (const MPlug& plug, MDataBlock& data)
{
	
	MStatus returnStatus;
 
	// Check that the requested recompute is one of the output values
	//
	if (plug == sOutput || plug == cOutput) {
		// Read the input values
		//
		MDataHandle inputData = data.inputValue (input, &returnStatus);
		CHECK_MSTATUS( returnStatus );
		MDataHandle scaleData = data.inputValue (scale, &returnStatus);
		CHECK_MSTATUS( returnStatus );
		MDataHandle framesData = data.inputValue (frames, &returnStatus);
		CHECK_MSTATUS( returnStatus );

		// Compute the output values
		//
		float currentFrame = inputData.asFloat();
		float scaleFactor  = scaleData.asFloat();
		float framesPerCircle = framesData.asFloat();
		float angle = 6.2831853f * (currentFrame/framesPerCircle);
		float sinResult = sinf (angle) * scaleFactor;
		float cosResult = cosf (angle) * scaleFactor;

		// Store them on the output plugs
		//
		MDataHandle sinHandle = data.outputValue( circle::sOutput,
				&returnStatus );
		CHECK_MSTATUS( returnStatus );
		MDataHandle cosHandle = data.outputValue( circle::cOutput,
				&returnStatus );
		CHECK_MSTATUS( returnStatus );
		sinHandle.set( sinResult );
		cosHandle.set( cosResult );
		CHECK_MSTATUS( data.setClean( plug ) );
	} else {
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

// The initializePlugin method is called by Maya when the circleNode
// plugin is loaded.  It registers the circleNode which provides
// Maya with the creator and initialize methods to be called when
// a circleNode is created.
//
MStatus initializePlugin ( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.5", "Any");

	status = plugin.registerNode( "circle", circle::id,
						  circle::creator, circle::initialize );
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

// The unitializePlugin is called when Maya needs to unload the plugin.
// It basically does the opposite of initialize by calling
// the deregisterCommand to remove it.
//
MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( circle::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
