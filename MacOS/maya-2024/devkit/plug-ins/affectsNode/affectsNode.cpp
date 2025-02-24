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

#include <maya/MIOStream.h>
#include <string.h>
#include <math.h>

#include <maya/MPxNode.h> 

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPlugin.h>

#include <maya/MString.h> 
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MVector.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
 
/////////////////////////////////
// Plugin Affects Class		   //
/////////////////////////////////

// INTRODUCTION:
//	This class will create an "affects" node. This node is used for
//	demonstrating attributeAffects relationships involving dynamic
//	attributes.
//
// WHAT THIS PLUG-IN DEMONSTRATES:
//	This plug-in creates a node called "affects". Add two dynamic
//	integer attributes called "A" and "B". When you change the value on
//	A, note that B will recompute.
//
// HOW TO USE THIS PLUG-IN:
//	(1) Compile the plug-in
//	(2) Load the compiled plug-in into Maya via the plug-in manager
//	(3) Create an "affects" node by typing the MEL command:
//			createNode affects;
//	(4) Add two integer dynamic attributes to the newly created
//		affects node by typing the MEL command:
//			addAttr -ln A -at long  affects1;
//			addAttr -ln B -at long  affects1;
//	(5) Change the value of "A" to 10 by typing the MEL command:
//			setAttr affects1.A 10;
//		At this point, the affectsNode::setDependentsDirty() method
//		gets called which causes "B" to be marked dirty.
//	(6) Compute the value on "B" by doing a getAttr:
//			getAttr affects1.B;
//		The affectsNode::compute() method is entered which copies the
//		value from "A" (i.e. 10) to "B".
//
class affects : public MPxNode
{
public:
						affects();
					~affects() override; 

	MStatus		compute( const MPlug& plug, MDataBlock& data ) override;
	MStatus		setDependentsDirty( const MPlug& plugBeingDirtied,
								MPlugArray &affectedPlugs ) override;

	static  void*		creator();
	static  MStatus		initialize();

	static	MTypeId		id;				// The IFF type id
};

// IFF type ID
// Each node requires a unique identifier which is used by
// MFnDependencyNode::create() to identify which node to create, and by
// the Maya file format.
//
// For local testing of nodes you can use any identifier between
// 0x00000000 and 0x0007ffff, but for any node that you plan to use for
// more permanent purposes, you should get a universally unique id from
// Autodesk Support. You will be assigned a unique range that you can manage
// on your own.
//
MTypeId affects::id( 0x80028 );

// This node does not need to perform any special actions on creation or
// destruction
//
affects::affects() {}
affects::~affects() {}

// The compute() method does the actual work of the node using the inputs
// of the node to generate its output.
//
// Compute takes two parameters: plug and data.
// - Plug is the the data value that needs to be recomputed
// - Data provides handles to all of the nodes attributes, only these
//   handles should be used when performing computations.
//
MStatus affects::compute( const MPlug& plug, MDataBlock& data )
{
	MStatus status;
	MObject thisNode = thisMObject();
	MFnDependencyNode fnThisNode( thisNode );
	fprintf(stderr,"affects::compute(), plug being computed is \"%s\"\n",
			plug.name().asChar());
 
	if ( plug.partialName() == "B" ) {
		// Plug "B" is being computed. Assign it the value on plug "A"
		// if "A" exists.
		//
		MPlug pA = fnThisNode.findPlug( "A",  true,  &status );
		if ( MStatus::kSuccess == status ) {
			fprintf(stderr,"\t\t... found dynamic attribute \"A\", copying its value to \"B\"\n");
			MDataHandle inputData = data.inputValue( pA, &status );
			CHECK_MSTATUS( status );
			int value = inputData.asInt();

			MDataHandle outputHandle = data.outputValue( plug );
			outputHandle.set( value );
			data.setClean(plug);
		}
	} else {
		return MS::kUnknownParameter;
	}
	return( MS::kSuccess );
}

// The creator() method allows Maya to instantiate instances of this node.
// It is called every time a new instance of the node is requested by
// either the createNode command or the MFnDependencyNode::create()
// method.
//
// In this case creator simply returns a new affects object.
//
void* affects::creator()
{
	return( new affects() );
}

// The initialize method is called only once when the node is first
// registered with Maya. In general,
//
MStatus affects::initialize()
{
	return( MS::kSuccess );
}

// The setDependentsDirty() method allows attributeAffects relationships
// in a much more general way than via MPxNode::attributeAffects
// which is limited to static attributes only.
// The setDependentsDirty() method allows relationships to be established
// between any combination of dynamic and static attributes.
//
// Within a setDependentsDirty() implementation you get passed in the
// plug which is being set dirty, and then, based upon which plug it is,
// you may choose to dirty any other plugs by adding them to the
// affectedPlugs list.
//
// In almost all cases, the relationships you set up will be fixed for
// the duration of Maya, such as "A affects B". However, you can also
// set up relationships which depend upon some external factor, such
// as the current frame number, the time of day, if maya was invoked in
// batch mode, etc. These sorts of relationships are straightforward to
// implement in your setDependentsDirty() method.
//
// There may also be situations where you need to look at values in the
// dependency graph. It is VERY IMPORTANT that when accessing DG values
// you do not cause a DG evaluation. This is because your setDependentsDirty()
// method is called during dirty processing and causing an evalutaion could
// put Maya into an infinite loop. The only safe way to look at values
// on plugs is via the MDataBlock::outputValue() which does not trigger
// an evaluation. It is recommeneded that you only look at plugs whose
// values are constant or you know have already been computed.
//
// For this example routine, we will only implement the simplest case
// of a relationship.
//
MStatus affects::setDependentsDirty( const MPlug &plugBeingDirtied,
		MPlugArray &affectedPlugs )
{
	MStatus	status;
	MObject thisNode = thisMObject();
	MFnDependencyNode fnThisNode( thisNode );

	if ( plugBeingDirtied.partialName() == "A" ) {
		// "A" is dirty, so mark "B" dirty if "B" exists.
		// This implements the relationship "A affects B".
		//
		fprintf(stderr,"affects::setDependentsDirty, \"A\" being dirtied\n");
		MPlug pB = fnThisNode.findPlug( "B",  true,  &status );
		if ( MStatus::kSuccess == status ) {
			fprintf(stderr,"\t\t... dirtying \"B\"\n");
			CHECK_MSTATUS( affectedPlugs.append( pB ) );
		}
	}
	return( MS::kSuccess );
}

// These methods load and unload the plugin, registerNode registers the
// new node type with maya
//
MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY , "6.0", "Any");

	status = plugin.registerNode( "affects", affects::id, affects::creator,
								  affects::initialize );
	if (!status) {
		status.perror("registerNode");
		return( status );
	}

	return( status );
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( affects::id );
	if (!status) {
		status.perror("deregisterNode");
		return( status );
	}

	return( status );
}
