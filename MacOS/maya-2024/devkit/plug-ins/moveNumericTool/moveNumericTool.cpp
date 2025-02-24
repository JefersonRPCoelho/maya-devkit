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
// 
// DESCRIPTION:
//
// Produces these MEL commands: moveNumericToolCmd and moveNumericToolContext. 
//    
// This is an interactive tool for moving an object.
//
// This is an example of a selection-action tool that allows the user to type
// in precise translation values while in the move tool. Once an object
// is selected, the tool turns into a translation tool.
// In this mode, the user can type in numeric values in the numeric input field
// to translate the object. 
// 
// This tool only supports the translation of transforms and will only perform translation
// in orthographic views. Undo, redo, and journaling are supported by this tool.
//
// To use this plug-in:
//	(1) Execute the command "source moveNumericTool".
//		This creates a new entry in the "Shelf1" tab of the tool shelf called "moveNumericTool".
//	(2) Click on the new icon, then select an object and drag it around in an orthographic view.
//	(3) Type in the numeric input field with the object still selected to enter a specific translation.
//
// You can click on the button to the left of the numeric input field to specify whether
// you want absolute or relative translation values.
//
////////////////////////////////////////////////////////////////////////

#include <maya/MIOStream.h>
#include <stdio.h>
#include <stdlib.h>

#include <maya/MPxToolCommand.h>
#include <maya/MFnPlugin.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>

#include <maya/MFnTransform.h>
#include <maya/MItCurveCV.h>
#include <maya/MItSurfaceCV.h>
#include <maya/MItMeshVertex.h>

#include <maya/MPxSelectionContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/M3dView.h>
#include <maya/MFnCamera.h>

#include <maya/MFeedbackLine.h>

#define CHECKRESULT(stat,msg)     \
	if ( MS::kSuccess != stat ) { \
		cerr << msg << endl;      \
	}

#define kVectorEpsilon 1.0e-3

/////////////////////////////////////////////////////////////
//
// The move command
//
// - this is a tool command which can be used in tool
//   contexts or in the MEL command window.
//
/////////////////////////////////////////////////////////////
#define		MOVENAME	"moveNumericToolCmd"
#define		DOIT		0
#define		UNDOIT		1
#define		REDOIT		2

class moveCmd : public MPxToolCommand
{
public:
	moveCmd();
	~moveCmd() override; 

	MStatus     	doIt( const MArgList& args ) override;
	MStatus     	redoIt() override;
	MStatus     	undoIt() override;
	bool        	isUndoable() const override;
	MStatus			finalize() override;
	
public:
	static void*	creator();

	void			setVector( double x, double y, double z );
	static MStatus	getVector( MVector &vec );
private:
	MVector 		delta;	// the delta vectors
	MStatus 		action( int flag );	// do the work here
};

moveCmd::moveCmd( )
{
	setCommandString( MOVENAME );
}

moveCmd::~moveCmd()
{}

void* moveCmd::creator()
{
	return new moveCmd;
}

bool moveCmd::isUndoable() const
//
// Description
//     Set this command to be undoable.
//
{
	return true;
}

void moveCmd::setVector( double x, double y, double z)
{
	delta.x = x;
	delta.y = y;
	delta.z = z;
}

MStatus moveCmd::getVector( MVector &vec )
{
	MStatus stat;
	MSelectionList slist;
 	MGlobal::getActiveSelectionList( slist );

	MDagPath 	mDagPath;
	MObject 	mComponent;
	MSpace::Space spc = MSpace::kWorld;

	// Translate first selected object
	//
	if (slist.length() > 0)
		stat = slist.getDagPath(slist.length()-1, mDagPath, mComponent);
	else
		return MS::kFailure;

	if ( MS::kSuccess != stat ) return MS::kFailure;

	MFnTransform transFn( mDagPath, &stat );
	if ( MS::kSuccess == stat ) {
		vec = transFn.getTranslation( spc, &stat );
	}

	return stat;
}

MStatus moveCmd::finalize()
//
// Description
//     Command is finished, construct a string for the command
//     for journalling.
//
{
    MArgList command;
    command.addArg( commandString() );
    command.addArg( delta.x );
    command.addArg( delta.y );
    command.addArg( delta.z );

	// This call adds the command to the undo queue and sets
	// the journal string for the command.
	//
    return MPxToolCommand::doFinalize( command );
}

MStatus moveCmd::doIt( const MArgList& args )
//
// Description
// 		Test MItSelectionList class
//
{
	MStatus stat;
	MVector	vector( 1.0, 0.0, 0.0 );	// default delta
	unsigned i = 0;

	switch ( args.length() )	 // set arguments to vector
	{
		case 1:
			vector.x = args.asDouble( 0, &stat );
			break;
		case 2:
			vector.x = args.asDouble( 0, &stat );
			vector.y = args.asDouble( 1, &stat );
			break;
		case 3:
			vector = args.asVector(i,3);
			break;
		case 0:
		default:
			break;
	}
	delta = vector;

	return action( DOIT );
}

MStatus moveCmd::undoIt( )
//
// Description
// 		Undo last delta translation
//
{
	return action( UNDOIT );
}

MStatus moveCmd::redoIt( )
//
// Description
// 		Redo last delta translation
//
{
	return action( REDOIT );
}

MStatus moveCmd::action( int flag )
//
// Description
// 		Do the actual work here to move the objects	by vector
//
{
	MStatus stat;
	MVector vector = delta;

	switch( flag )
	{
		case UNDOIT:	// undo
			vector.x = -vector.x;
			vector.y = -vector.y;
			vector.z = -vector.z;
			break;
		case REDOIT:	// redo
			break;
		case DOIT:		// do command
			break;
		default:
			break;
	}

	// Create a selection list iterator
	//
	MSelectionList slist;
 	MGlobal::getActiveSelectionList( slist );

	MDagPath 	mdagPath;		// Item dag path
	MObject 	mComponent;		// Current component
	MSpace::Space spc = MSpace::kWorld;

	// Translate first selected objects
	//
	if (slist.length() > 0)
		stat = slist.getDagPath(slist.length()-1, mdagPath, mComponent);
	else
		return MS::kFailure;

	if ( MS::kSuccess != stat ) return MS::kFailure;

	MFnTransform transFn( mdagPath, &stat );
	if ( MS::kSuccess == stat ) {
		stat = transFn.translateBy( vector, spc );
		CHECKRESULT(stat,"Error doing translate on transform");
	}

	return MS::kSuccess;
}


/////////////////////////////////////////////////////////////
//
// The moveNumericTool Context
//
// - tool contexts are custom event handlers. The selection
//   context class defaults to maya's selection mode and
//   allows you to override press/drag/release events.
//
/////////////////////////////////////////////////////////////
#define     MOVEHELPSTR        "drag to move selected object"
#define     MOVETITLESTR       "moveNumericTool"
#define		TOP			0
#define		FRONT		1
#define		SIDE		2
#define		PERSP		3

class moveNumericContext : public MPxSelectionContext
{
public:
    moveNumericContext();
    void    toolOnSetup( MEvent & event ) override;
    MStatus doPress( MEvent & event ) override;
    MStatus doDrag( MEvent & event ) override;
    MStatus doRelease( MEvent & event ) override;
    MStatus doEnterRegion( MEvent & event ) override;

	bool		processNumericalInput( const MDoubleArray &values,
											   const MIntArray &flags,
											   bool isAbsolute ) override;
	bool		feedbackNumericalInput() const override;
	MSyntax::MArgType	argTypeNumericalInput( unsigned index ) const override;

private:
	int currWin;
	MEvent::MouseButtonType downButton;
	M3dView view;
	short startPos_x, endPos_x, start_x, last_x;
	short startPos_y, endPos_y, start_y, last_y;
	moveCmd * cmd;
};


moveNumericContext::moveNumericContext()
{
	MString str( MOVETITLESTR );
    setTitleString( str );

	// Tell the context which XPM to use so the tool can properly
	// be a candidate for the 6th position on the mini-bar.
	setImage("moveNumericTool.xpm", MPxContext::kImage1 );
}

void moveNumericContext::toolOnSetup( MEvent & )
{
	MString str( MOVEHELPSTR );
    setHelpString( str );
}

MStatus moveNumericContext::doPress( MEvent & event )
{
	MStatus stat = MPxSelectionContext::doPress( event );
	MSpace::Space spc = MSpace::kWorld;

	// If we are not in selecting mode (i.e. an object has been selected)
	// then set up for the translation.
	//
	if ( !isSelecting() ) {
		event.getPosition( startPos_x, startPos_y );
		view = M3dView::active3dView();

		MDagPath camera;
		stat = view.getCamera( camera );
		if ( stat != MS::kSuccess ) {
			cerr << "Error: M3dView::getCamera" << endl;
			return stat;
		}
		MFnCamera fnCamera( camera );
		MVector upDir = fnCamera.upDirection( spc );
		MVector rightDir = fnCamera.rightDirection( spc );

		// Determine the camera used in the current view
		//
		if ( fnCamera.isOrtho() ) {
			if ( upDir.isEquivalent(MVector::zNegAxis,kVectorEpsilon) ) {
				currWin = TOP;
			} else if (rightDir.isEquivalent(MVector::xAxis,kVectorEpsilon)) {
				currWin = FRONT;
			} else  {
				currWin = SIDE;
			}
		}
		else {
			currWin = PERSP;
		}

		// Create an instance of the move tool command.
		//
		cmd = (moveCmd*)newToolCommand();

		cmd->setVector( 0.0, 0.0, 0.0 );
	}
	feedbackNumericalInput();
	return stat;
}

MStatus moveNumericContext::doDrag( MEvent & event )
{
	MStatus stat;
	stat = MPxSelectionContext::doDrag( event );

	// If we are not in selecting mode (i.e. an object has been selected)
	// then do the translation.
	//
	if ( !isSelecting() ) {
		event.getPosition( endPos_x, endPos_y );
		MPoint endW, startW;
		MVector vec;
		view.viewToWorld( startPos_x, startPos_y, startW, vec );
		view.viewToWorld( endPos_x, endPos_y, endW, vec );
		downButton = event.mouseButton();

		// We reset the the move vector each time a drag event occurs 
		// and then recalculate it based on the start position. 
		//
		cmd->undoIt();

		switch( currWin )
		{
			case TOP:
				switch ( downButton )
				{
					case MEvent::kMiddleMouse :
						cmd->setVector( endW.x - startW.x, 0.0, 0.0 );
						break;
					case MEvent::kLeftMouse :
					default:
						cmd->setVector( endW.x - startW.x, 0.0,
										endW.z - startW.z );
						break;
				}
				break;	

			case FRONT:
				switch ( downButton )
				{
					case MEvent::kMiddleMouse :
						cmd->setVector( endW.x - startW.x, 0.0, 0.0 );
						break;
					case MEvent::kLeftMouse :
					default:
						cmd->setVector( endW.x - startW.x,
										endW.y - startW.y, 0.0 );
						break;
				}
				break;	

			case SIDE:
				switch ( downButton )
				{
					case MEvent::kMiddleMouse :
						cmd->setVector( 0.0, 0.0, endW.z - startW.z );
						break;
					case MEvent::kLeftMouse :
					default:
						cmd->setVector( 0.0, endW.y - startW.y,
										endW.z - startW.z );
						break;
				}
				break;	

			case PERSP:
				break;
		}

		stat = cmd->redoIt();
		view.refresh( true );
	}
	feedbackNumericalInput();
	return stat;
}

MStatus moveNumericContext::doRelease( MEvent & event )
{
	MStatus stat = MPxSelectionContext::doRelease( event );
	if ( !isSelecting() ) {
		event.getPosition( endPos_x, endPos_y );

		// Delete the move command if we have moved less then 2 pixels
		// otherwise call finalize to set up the journal and add the
		// command to the undo queue.
		//
		if (abs(startPos_x - endPos_x) < 2 && abs(startPos_y - endPos_y) < 2) {
			delete cmd;
			view.refresh( true );
		}
		else {
			stat = cmd->finalize();
			view.refresh( true );
		}
	}
	feedbackNumericalInput();
	return stat;
}

MStatus moveNumericContext::doEnterRegion( MEvent & /*event*/ )
//
// Print the tool description in the help line.
//
{
	MString str( MOVEHELPSTR );
    return setHelpString( str );
}


bool moveNumericContext::processNumericalInput ( const MDoubleArray &values,
												 const MIntArray &flags,
												 bool isAbsolute )
{
	unsigned valueLength = values.length();

	cmd = (moveCmd *) newToolCommand();

	MVector vec;
	/* MStatus stat = */ moveCmd::getVector(vec);

	if (isAbsolute) {
		MVector absoluteDelta;

		if (ignoreEntry(flags, 0) || (valueLength < 1)) absoluteDelta.x = 0;
		else absoluteDelta.x = values[0] - vec.x;

		if (ignoreEntry(flags, 1) || (valueLength < 2)) absoluteDelta.y = 0;
		else absoluteDelta.y = values[1] - vec.y;

		if (ignoreEntry(flags, 2) || (valueLength < 3)) absoluteDelta.z = 0;
		else absoluteDelta.z = values[2] - vec.z;

		cmd->setVector(absoluteDelta.x, absoluteDelta.y, absoluteDelta.z);
	}
	else {
		MVector relativeDelta;

		if (ignoreEntry(flags, 0) || (valueLength < 1)) relativeDelta.x = 0;
		else relativeDelta.x = values[0];

		if (ignoreEntry(flags, 1) || (valueLength < 2)) relativeDelta.y = 0;
		else relativeDelta.y = values[1];

		if (ignoreEntry(flags, 2) || (valueLength < 3)) relativeDelta.z = 0;
		else relativeDelta.z = values[2];

		cmd->setVector(relativeDelta.x, relativeDelta.y, relativeDelta.z);
	}

	cmd->redoIt();
	cmd->finalize();

	feedbackNumericalInput();
	return true;
}

bool moveNumericContext::feedbackNumericalInput() const
{
	MFeedbackLine::setTitle("moveNumericTool");
	MFeedbackLine::setFormat("^6.3f ^6.3f ^6.3f");
	MVector vec;
	/* MStatus stat = */ moveCmd::getVector(vec);
	MFeedbackLine::setValue(0, vec.x);
	MFeedbackLine::setValue(1, vec.y);
	MFeedbackLine::setValue(2, vec.z);
	return true;
}

MSyntax::MArgType moveNumericContext::argTypeNumericalInput(unsigned /*index*/) 
const
{
	return MSyntax::kDistance;
}


/////////////////////////////////////////////////////////////
//
// Context creation command
//
//  This is the command that will be used to create instances
//  of our context.
//
/////////////////////////////////////////////////////////////
#define     CREATE_CTX_NAME	"moveNumericToolContext"

class moveNumericContextCommand : public MPxContextCommand
{
public:
    moveNumericContextCommand() {};
    MPxContext * makeObj() override;

public:
    static void* creator();
};

MPxContext * moveNumericContextCommand::makeObj()
{
    return new moveNumericContext();
}

void * moveNumericContextCommand::creator()
{
    return new moveNumericContextCommand;
}


///////////////////////////////////////////////////////////////////////
//
// The following routines are used to register/unregister
// the commands we are creating within Maya
//
///////////////////////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
	MStatus status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any" );

	status = plugin.registerContextCommand(CREATE_CTX_NAME,
										   &moveNumericContextCommand::creator,
										   MOVENAME, &moveCmd::creator);
	if (!status) {
		status.perror("registerContextCommand");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterContextCommand( CREATE_CTX_NAME, MOVENAME );
	if (!status) {
		status.perror("deregisterContextCommand");
		return status;
	}

	return status;
}
