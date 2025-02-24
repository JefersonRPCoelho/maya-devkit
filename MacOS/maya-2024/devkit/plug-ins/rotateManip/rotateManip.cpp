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
// This example produces the MEL command "rotateContext" to create the tool context
// for this manipulator. 
//
// This is an example to demonstrate the use of a rotation manipulator through
// a rotation tool and context. This example uses three classes to accomplish
// this task: First, a context command (rotateContext) is provided to create
// instances of the context.  Next, a custom selection context (RotateManipContext)
// is created to manage the rotation manipulator. Finally, the rotation manipulator
// is provided as a custom node class.
//
// Loading and unloading:
//
// The rotate manipulator context can be created with the following mel commands:
//    
//	rotateContext;
//	setToolTo rotateContext1;
//
// If the preceding commands were used to create the manipulator context,
// the following commands can destroy it:
//
//	deleteUI rotateContext1;
//	deleteUI rotateManip;
//
// If the plugin is loaded and unloaded frequently (such as during testing),
// it is useful to make these command sequences into shelf buttons.
//
// To create the tool button for the plug-in, create a new shelf named "Shelf1"
// and execute the following MEL commands to create the tool button in this shelf: 
//
//	rotateContext;
//	setParent Shelf1;
//	toolButton -cl toolCluster -t rotateContext1 -i1 "moveManip.xpm";
//
// How to use:
//
// To use the manipulator, select an object and click on the new tool button.
// A rotate manipulator should appear on the object along with a state manipulator nearby.
// The state manipulator should be displayed 2 units along the X-axis from the object.
//
// The plug-in rotate manipulator is configured to behave similarly to the built-in
// rotate manipulator. The state manipulator can be used to choose the mode for
// the rotate manipulator, which can be one of object space mode, world space mode,
// gimbal mode, and object space mode with snapping enabled.
//
////////////////////////////////////////////////////////////////////////

#include <maya/MIOStream.h>
#include <stdio.h>
#include <stdlib.h>

#include <maya/MFn.h>
#include <maya/MPxNode.h>
#include <maya/MPxManipContainer.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MModelMessage.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>
#include <maya/MManipData.h>
#include <maya/MEulerRotation.h>

// Manipulators
#include <maya/MFnRotateManip.h>
#include <maya/MFnStateManip.h>


// This function is a utility that can be used to extract vector values from
// plugs.
//
MVector vectorPlugValue(const MPlug& plug) {
	if (plug.numChildren() == 3)
	{
		double x,y,z;
		MPlug rx = plug.child(0);
		MPlug ry = plug.child(1);
		MPlug rz = plug.child(2);
		rx.getValue(x);
		ry.getValue(y);
		rz.getValue(z);
		MVector result(x,y,z);
		return result;
	}
	else {
		MGlobal::displayError("Expected 3 children for plug "+MString(plug.name()));
		MVector result(0,0,0);
		return result;
	}
}

/////////////////////////////////////////////////////////////
//
// exampleRotateManip
//
// This class implements the example rotate manipulator.
//
/////////////////////////////////////////////////////////////

class exampleRotateManip : public MPxManipContainer
{
public:
	exampleRotateManip();
	~exampleRotateManip() override;
	
	static void * creator();
	static MStatus initialize();
	MStatus createChildren() override;
	MStatus connectToDependNode(const MObject &node) override;

	// Callback function
	MManipData rotationChangedCallback(unsigned index);

public:
	static MTypeId id;

private:
	MDagPath fRotateManip;
	MDagPath fStateManip;

	unsigned rotatePlugIndex;
};


MTypeId exampleRotateManip::id( 0x80022 );

exampleRotateManip::exampleRotateManip() 
{ 
	// The constructor must not call createChildren for user-defined
	// manipulators.
}

exampleRotateManip::~exampleRotateManip() 
{
}


void *exampleRotateManip::creator()
{
	 return new exampleRotateManip();
}


MStatus exampleRotateManip::initialize()
{
	return MPxManipContainer::initialize();
}


MStatus exampleRotateManip::createChildren()
{
	MStatus stat = MStatus::kSuccess;

	// Add the rotation manip
	//
	fRotateManip = addRotateManip("RotateManip", "rotation");

	// Add the state manip.  The state manip is used to cycle through the 
	// rotate manipulator modes to demonstrate how they work.
	//
	fStateManip = addStateManip("StateManip", "state");

	// The state manip permits 4 states.  These correspond to:
	// 0 - Rotate manip in objectSpace mode
	// 1 - Rotate manip in worldSpace mode
	// 2 - Rotate manip in gimbal mode
	// 3 - Rotate manip in objectSpace mode with snapping on
	//
	// Note that while the objectSpace and gimbal modes will operator similar 
	// to the built-in Maya rotate manipulator, the worldSpace mode will 
	// produce unusual rotations because the plugin does not convert worldSpace
	// rotations to object space.
	//
	MFnStateManip stateManip(fStateManip);
	stateManip.setMaxStates(4);
	stateManip.setInitialState(0);
	
	return stat;
}


MStatus exampleRotateManip::connectToDependNode(const MObject &node)
{
	MStatus stat;

	// Find the rotate and rotatePivot plugs on the node.  These plugs will 
	// be attached either directly or indirectly to the manip values on the
	// rotate manip.
	//
	MFnDependencyNode nodeFn(node);
	MPlug rPlug = nodeFn.findPlug("rotate",  true,  &stat);
	if (!stat)
	{
		MGlobal::displayError("Could not find rotate plug on node");
		return stat;
	}
	MPlug rcPlug = nodeFn.findPlug("rotatePivot",  true,  &stat);
	if (!stat)
	{
		MGlobal::displayError("Could not find rotatePivot plug on node");
		return stat;
	}

	// If the translate pivot exists, it will be used to move the state manip
	// to a convenient location.
	//
	MPlug tPlug = nodeFn.findPlug("translate",  true,  &stat);

	// To avoid having the object jump back to the default rotation when the
	// manipulator is first used, extract the existing rotation from the node
	// and set it as the initial rotation on the manipulator.
	//
	MEulerRotation existingRotation(vectorPlugValue(rPlug));
	MVector existingTranslation(vectorPlugValue(tPlug));

	// 
	// The following code configures default settings for the rotate 
	// manipulator.
	//

	MFnRotateManip rotateManip(fRotateManip);
	rotateManip.setInitialRotation(existingRotation);
	rotateManip.setRotateMode(MFnRotateManip::kObjectSpace);
	rotateManip.displayWithNode(node);

	// Add a callback function to be called when the rotation value changes
	//
	rotatePlugIndex = addManipToPlugConversionCallback( rPlug, 
		(manipToPlugConversionCallback)
		&exampleRotateManip::rotationChangedCallback );

	// Create a direct (1-1) connection to the rotation center plug
	//
	rotateManip.connectToRotationCenterPlug(rcPlug);

	// Place the state manip at a distance of 2.0 units away from the object
	// along the X-axis.
	//
	MFnStateManip stateManip(fStateManip);
	stateManip.setTranslation(existingTranslation+MVector(2,0,0),
		MSpace::kTransform);

	// add the rotate XYZ plugs to the In-View Editor
	//
	MPlug rxPlug = rPlug.child( 0 );
	addPlugToInViewEditor( rxPlug );
	MPlug ryPlug = rPlug.child( 1 );
	addPlugToInViewEditor( ryPlug );
	MPlug rzPlug = rPlug.child( 2 );
	addPlugToInViewEditor( rzPlug );

	finishAddingManips();
	MPxManipContainer::connectToDependNode(node);
	return stat;
}

MManipData exampleRotateManip::rotationChangedCallback(unsigned index) {
	static MEulerRotation cache;
	MObject obj = MObject::kNullObj;

	// If we entered the callback with an invalid index, print an error and
	// return.  Since we registered the callback only for one plug, all 
	// invocations of the callback should be for that plug.
	//
	if (index != rotatePlugIndex)
	{
		MGlobal::displayError("Invalid index in rotation changed callback!");

		// For invalid indices, return vector of 0's
		MFnNumericData numericData;
		obj = numericData.create( MFnNumericData::k3Double );
		numericData.setData(0.0,0.0,0.0);

		return obj;
	}

	// Assign function sets to the manipulators
	//
	MFnStateManip stateManip(fStateManip);
	MFnRotateManip rotateManip(fRotateManip);

	// Adjust settings on the rotate manip based on the state of the state 
	// manip.
	//
	int mode = stateManip.state();
	if (mode != 3)
	{
		rotateManip.setRotateMode((MFnRotateManip::RotateMode) stateManip.state());
		rotateManip.setSnapMode(false);
	}
	else {
		// State 3 enables snapping for an object space manip.  In this case,
		// we snap every 15.0 degrees.
		//
		rotateManip.setRotateMode(MFnRotateManip::kObjectSpace);
		rotateManip.setSnapMode(true);
		rotateManip.setSnapIncrement(15.0);
	}

	// The following code creates a data object to be returned in the 
	// MManipData.  In this case, the plug to be computed must be a 3-component
	// vector, so create data as MFnNumericData::k3Double
	//
	MFnNumericData numericData;
	obj = numericData.create( MFnNumericData::k3Double );

	// Retrieve the value for the rotation from the manipulator and return it
	// directly without modification.  If the manipulator should eg. slow down
	// rotation, this method would need to do some math with the value before
	// returning it.
	//
	MEulerRotation manipRotation;
	if (!getConverterManipValue (rotateManip.rotationIndex(), manipRotation))
	{
		MGlobal::displayError("Error retrieving manip value");
		numericData.setData(0.0,0.0,0.0);
	}
	else {
		numericData.setData(manipRotation.x, manipRotation.y, manipRotation.z);
	}

	return MManipData(obj);
}

/////////////////////////////////////////////////////////////
//
// RotateManipContext
//
// This class is a simple context for supporting a rotate manipulator.
//
/////////////////////////////////////////////////////////////

class RotateManipContext : public MPxSelectionContext
{
public:
	RotateManipContext();
	void	toolOnSetup(MEvent &event) override;
	void	toolOffCleanup() override;

	// Callback issued when selection list changes
	static void updateManipulators(void * data);

private:
	MCallbackId id1;
};

RotateManipContext::RotateManipContext()
{
	MString str("Plugin Rotate Manipulator");
	setTitleString(str);
}


void RotateManipContext::toolOnSetup(MEvent &)
{
	MString str("Rotate the object using the rotation handles");
	setHelpString(str);

	updateManipulators(this);
	MStatus status;
	id1 = MModelMessage::addCallback(MModelMessage::kActiveListModified,
									 updateManipulators, 
									 this, &status);
	if (!status) {
		MGlobal::displayError("Model addCallback failed");
	}
}


void RotateManipContext::toolOffCleanup()
{
	MStatus status;
	status = MModelMessage::removeCallback(id1);
	if (!status) {
		MGlobal::displayError("Model remove callback failed");
	}
	MPxContext::toolOffCleanup();
}


void RotateManipContext::updateManipulators(void * data)
{
	MStatus stat = MStatus::kSuccess;
	
	RotateManipContext * ctxPtr = (RotateManipContext *) data;
	ctxPtr->deleteManipulators(); 

	// Add the rotate manipulator to each selected object.  This produces 
	// behavior different from the default rotate manipulator behavior.  Here,
	// a distinct rotate manipulator is attached to every object.
	// 
	MSelectionList list;
	stat = MGlobal::getActiveSelectionList(list);
	MItSelectionList iter(list, MFn::kInvalid, &stat);

	if (MS::kSuccess == stat) {
		for (; !iter.isDone(); iter.next()) {

			// Make sure the selection list item is a depend node and has the
			// required plugs before manipulating it.
			//
			MObject dependNode;
			iter.getDependNode(dependNode);
			if (dependNode.isNull() || !dependNode.hasFn(MFn::kDependencyNode))
			{
				MGlobal::displayWarning("Object in selection list is not "
					"a depend node.");
				continue;
			}

			MFnDependencyNode dependNodeFn(dependNode);
			/* MPlug rPlug = */ dependNodeFn.findPlug("rotate",  true,  &stat);
			if (!stat) {
				MGlobal::displayWarning("Object cannot be manipulated: " +
					dependNodeFn.name());
				continue;
			}

			// Add manipulator to the selected object
			//
			MString manipName ("exampleRotateManip");
			MObject manipObject;
			exampleRotateManip* manipulator =
				(exampleRotateManip *) exampleRotateManip::newManipulator(
					manipName, 
					manipObject);

			if (NULL != manipulator) {
				// Add the manipulator
				//
				ctxPtr->addManipulator(manipObject);

				// Connect the manipulator to the object in the selection list.
				//
				if (!manipulator->connectToDependNode(dependNode))
				{
					MGlobal::displayWarning("Error connecting manipulator to"
						" object: " + dependNodeFn.name());
				}
			} 
		}
	}
}


/////////////////////////////////////////////////////////////
//
// rotateContext
//
// This is the command that will be used to create instances
// of our context.
//
/////////////////////////////////////////////////////////////

class rotateContext : public MPxContextCommand
{
public:
	rotateContext() {};
	MPxContext * makeObj() override;

public:
	static void* creator();
};

MPxContext *rotateContext::makeObj()
{
	return new RotateManipContext();
}

void *rotateContext::creator()
{
	return new rotateContext;
}


///////////////////////////////////////////////////////////////////////
//
// The following routines are used to register/unregister
// the context and manipulator
//
///////////////////////////////////////////////////////////////////////

MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "6.0", "Any");

	status = plugin.registerContextCommand("rotateContext",
										   &rotateContext::creator);
	if (!status) {
		MGlobal::displayError("Error registering rotateContext command");
		return status;
	}

	status = plugin.registerNode("exampleRotateManip", exampleRotateManip::id, 
								 &exampleRotateManip::creator, &exampleRotateManip::initialize,
								 MPxNode::kManipContainer);
	if (!status) {
		MGlobal::displayError("Error registering rotateManip node");
		return status;
	}

	return status;
}


MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterContextCommand("rotateContext");
	if (!status) {
		MGlobal::displayError("Error deregistering rotateContext command");
		return status;
	}

	status = plugin.deregisterNode(exampleRotateManip::id);
	if (!status) {
		MGlobal::displayError("Error deregistering RotateManip node");
		return status;
	}

	return status;
}
