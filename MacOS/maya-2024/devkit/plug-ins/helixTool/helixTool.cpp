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
//
// DESCRIPTION:
//
// Interactive tool to draw a helix. Uses OpenGL to draw a guideline for the helix.
//
// Produces the MEL commands: helixToolCmd and helixToolContext.
//
// This example takes the helix example one large step forward by wrapping the command in a context.
// This allows you to drag out the region in which you want the helix drawn.
// To use it, you must first execute the command "source helixTool".
// This will create a new entry in the "Shelf1" tab of the tool shelf called "Helix Tool".
// Click on the new icon, then move the cursor into the perspective window and drag out a cylinder
// which defines the volume in which the helix will be generated.
// This plug-in is an example of building a context around a command. 
//
// To create a tool command:
//
//	(1) Create a tool command class. 
//		Same process as an MPxCommand except define 2 methods
//		for interactive use: cancel and finalize.
//		There is also an addition constructor MPxToolCommand(), which 
//		is called from your context when the command needs to be invoked.
//
//	(2) Define your context.
//		This is accomplished by deriving off of MPxContext and overriding
//		whatever methods you need.
//
//	(3) Create a command class to create your context.
//		You will call this command in Maya to create and name a context.
//
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <maya/MIOStream.h>
#include <math.h>

#include <maya/MString.h>
#include <maya/MArgList.h>
#include <maya/MEvent.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MDagPath.h>

#include <maya/MPxContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MPxToolCommand.h> 
#include <maya/MToolsInfo.h>

#include <maya/MFnPlugin.h>
#include <maya/MFnNurbsCurve.h> 

#include <maya/MSyntax.h>
#include <maya/MArgParser.h>
#include <maya/MArgDatabase.h>
#include <maya/MCursor.h>

#include <maya/MGL.h>

#define kPitchFlag			"-p"
#define kPitchFlagLong		"-pitch"
#define kRadiusFlag			"-r"
#define kRadiusFlagLong		"-radius"
#define kNumberCVsFlag		"-ncv"
#define kNumberCVsFlagLong	"-numCVs"
#define kUpsideDownFlag		"-ud"
#define kUpsideDownFlagLong	"-upsideDown"

/////////////////////////////////////////////////////////////
// The users tool command
/////////////////////////////////////////////////////////////

#define		NUMBER_OF_CVS		20

class helixTool : public MPxToolCommand
{
public:
					helixTool(); 
				~helixTool() override; 
	static void*	creator();

	MStatus			doIt(const MArgList& args) override;
	MStatus			parseArgs(const MArgList& args);
	MStatus			redoIt() override;
	MStatus			undoIt() override;
	bool			isUndoable() const override;
	MStatus			finalize() override;
	static MSyntax	newSyntax();
	
	void			setRadius(double newRadius);
	void			setPitch(double newPitch);
	void			setNumCVs(unsigned newNumCVs);
	void			setUpsideDown(bool newUpsideDown);

private:
	double			radius;     	// Helix radius
	double			pitch;      	// Helix pitch
	unsigned		numCV;			// Helix number of CVs
	bool			upDown;			// Helix upsideDown
	MDagPath		path;			// The dag path to the curve.
									// Don't save the pointer!
};


void* helixTool::creator()
{
	return new helixTool;
}

helixTool::~helixTool() {}

helixTool::helixTool()
{
	numCV = 20;
	upDown = false;
	setCommandString("helixToolCmd");
}
	
MSyntax helixTool::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag(kPitchFlag, kPitchFlagLong, MSyntax::kDouble);
	syntax.addFlag(kRadiusFlag, kRadiusFlagLong, MSyntax::kDouble);
	syntax.addFlag(kNumberCVsFlag, kNumberCVsFlagLong, MSyntax::kUnsigned);
	syntax.addFlag(kUpsideDownFlag, kUpsideDownFlagLong, MSyntax::kBoolean);
	
	return syntax;
}

MStatus helixTool::doIt(const MArgList &args)
//
// Description
//     Sets up the helix parameters from arguments passed to the
//     MEL command.
//
{
	MStatus status;

	status = parseArgs(args);

	if (MS::kSuccess != status)
		return status;

	return redoIt();
}

MStatus helixTool::parseArgs(const MArgList &args)
{
	MStatus status;
	MArgDatabase argData(syntax(), args);

	if (argData.isFlagSet(kPitchFlag)) {
		double tmp;
		status = argData.getFlagArgument(kPitchFlag, 0, tmp);
		if (!status) {
			status.perror("pitch flag parsing failed");
			return status;
		}
		pitch = tmp;
	}
	
	if (argData.isFlagSet(kRadiusFlag)) {
		double tmp;
		status = argData.getFlagArgument(kRadiusFlag, 0, tmp);
		if (!status) {
			status.perror("radius flag parsing failed");
			return status;
		}
		radius = tmp;
	}
	
	if (argData.isFlagSet(kNumberCVsFlag)) {
		unsigned tmp;
		status = argData.getFlagArgument(kNumberCVsFlag, 0, tmp);
		if (!status) {
			status.perror("numCVs flag parsing failed");
			return status;
		}
		numCV = tmp;
	}
	
	if (argData.isFlagSet(kUpsideDownFlag)) {
		bool tmp;
		status = argData.getFlagArgument(kUpsideDownFlag, 0, tmp);
		if (!status) {
			status.perror("upside down flag parsing failed");
			return status;
		}
		upDown = tmp;
	}

	return MS::kSuccess;
}	


MStatus helixTool::redoIt()
//
// Description
//     This method creates the helix curve from the
//     pitch and radius values
//
{
	MStatus stat;

	const unsigned  deg     = 3;            // Curve Degree
	const unsigned  ncvs    = numCV;		// Number of CVs
	const unsigned  spans   = ncvs - deg;   // Number of spans
	const unsigned  nknots  = spans+2*deg-1;// Number of knots
	unsigned	    i;
	MPointArray		controlVertices;
	MDoubleArray	knotSequences;

	int upFactor;
	if (upDown) upFactor = -1;
	else upFactor = 1;

	// Set up cvs and knots for the helix
	//
	for (i = 0; i < ncvs; i++)
		controlVertices.append(MPoint(radius * cos((double) i),
									  upFactor * pitch * (double) i, 
									  radius * sin((double) i)));

	for (i = 0; i < nknots; i++)
		knotSequences.append((double) i);

	// Now create the curve
	//
	MFnNurbsCurve curveFn;

	curveFn.create(controlVertices, knotSequences, deg, 
				   MFnNurbsCurve::kOpen, false, false, 
				   MObject::kNullObj, &stat);

	if (!stat) {
		stat.perror("Error creating curve");
		return stat;
	}

	stat = curveFn.getPath( path );

	return stat;
}

MStatus helixTool::undoIt()
//
// Description
//     Removes the helix curve from the model.
//
{
	MStatus stat; 
	MObject transform = path.transform();
	stat = MGlobal::deleteNode( transform );
	return stat;
}

bool helixTool::isUndoable() const
//
// Description
//     Set this command to be undoable.
//
{
	return true;	
}

MStatus helixTool::finalize()
//
// Description
//     Command is finished, construct a string for the command
//     for journaling.
//
{
	MArgList command;
	command.addArg(commandString());
	command.addArg(MString(kRadiusFlag));
	command.addArg(radius);
	command.addArg(MString(kPitchFlag));
	command.addArg(pitch);
	command.addArg(MString(kNumberCVsFlag));
	command.addArg((int) numCV);
	command.addArg(MString(kUpsideDownFlag));
	command.addArg(upDown);
	return MPxToolCommand::doFinalize( command );
}

void helixTool::setRadius(double newRadius)
{
	radius = newRadius;
}

void helixTool::setPitch(double newPitch)
{
	pitch = newPitch;
}

void helixTool::setNumCVs(unsigned newNumCVs)
{
	numCV = newNumCVs;
}

void helixTool::setUpsideDown(bool newUpsideDown)
{
	upDown = newUpsideDown;
}


/////////////////////////////////////////////////////////////
//
// The user Context
//
//   Contexts give the user the ability to write functions
//   for handling events.
//
//   Contexts aren't registered in the plugin, instead a
//   command class (MPxContextCommand) is registered and is used
//   to create instances of the context.
//
/////////////////////////////////////////////////////////////

const char helpString[] = "Click and drag to draw helix";

class helixContext : public MPxContext
{
public:
					helixContext();
	void	toolOnSetup(MEvent &event) override;
	MStatus doPress(MEvent &event) override;
	MStatus doDrag(MEvent &event) override;
	MStatus doRelease(MEvent &event) override;
	MStatus doEnterRegion(MEvent &event) override;

		void	getClassName(MString & name) const override;

	void			setNumCVs(unsigned newNumCVs);
	void			setUpsideDown(bool newUpsideDown);
	unsigned		numCVs();
	bool			upsideDown();

private:
	void			drawGuide();

	bool			firstDraw;
	short			startPos_x, startPos_y;
	short			endPos_x, endPos_y;
	unsigned		numCV;
	bool			upDown;
	M3dView			view;
	GLdouble		height,radius;
	
};

helixContext::helixContext() 
{
	numCV = 20;
	upDown = false;
	setTitleString("Helix Tool");

	setCursor( MCursor::defaultCursor );

	// Tell the context which XPM to use so the tool can properly
	// be a candidate for the 6th position on the mini-bar.
	setImage("helixTool.xpm", MPxContext::kImage1 );
}

void helixContext::toolOnSetup(MEvent &)
{
	setHelpString(helpString);
}

MStatus helixContext::doPress(MEvent &event)
{
    event.getPosition(startPos_x, startPos_y);
    view = M3dView::active3dView();
	firstDraw = true;
    return MS::kSuccess;
}


void helixContext::drawGuide()
{
	int upFactor;
	if (upDown) upFactor = 1;
	else upFactor = -1;

    // Draw the guide cylinder
    //
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
        glRotatef(upFactor*90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadricObj *qobj = gluNewQuadric();
        gluQuadricDrawStyle(qobj, GLU_LINE);
        GLdouble factor = (GLdouble)numCV;
        radius = double(abs(endPos_x - startPos_x))/factor + 0.1;
        height = double(abs(endPos_y - startPos_y))/factor + 0.1;
        gluCylinder( qobj, radius, radius, height, 8, 1 );
    glPopMatrix();
}

MStatus helixContext::doDrag(MEvent & event)
{
	view.beginXorDrawing(false);

	if (!firstDraw) {
		//	Clear the guide from the old position.
		drawGuide();
	} else {
		firstDraw = false;
	}

    event.getPosition(endPos_x, endPos_y);

	//	Draw the guide at the new position.
	drawGuide();

	view.endXorDrawing();

	return MS::kSuccess;
}

MStatus helixContext::doRelease( MEvent & )
{
	//	Clear the guide from its last position.
	if (!firstDraw) {
		view.beginXorDrawing(false);
		drawGuide();
		view.endXorDrawing();
	}

	helixTool * cmd = (helixTool*)newToolCommand();
	cmd->setPitch( height/numCV );
	cmd->setRadius( radius );
	cmd->setNumCVs( numCV );
	cmd->setUpsideDown( upDown );
	cmd->redoIt();
	cmd->finalize();
	return MS::kSuccess;
}

MStatus helixContext::doEnterRegion( MEvent & )
{
	return setHelpString( helpString );
}

void helixContext::getClassName( MString & name ) const
{
	name.set("helix");
}

void helixContext::setNumCVs( unsigned newNumCVs )
{
	numCV = newNumCVs;
	MToolsInfo::setDirtyFlag(*this);
}

void helixContext::setUpsideDown( bool newUpsideDown )
{
	upDown = newUpsideDown;
	MToolsInfo::setDirtyFlag(*this);
}

unsigned helixContext::numCVs()
{
	return numCV;
}

bool helixContext::upsideDown()
{
	return upDown;
}

/////////////////////////////////////////////////////////////
//
// Context creation command
//
//  This is the command that will be used to create instances
//  of our context.
//
/////////////////////////////////////////////////////////////


class helixContextCmd : public MPxContextCommand
{
public:	
						helixContextCmd();
		MStatus		doEditFlags() override;
	MStatus		doQueryFlags() override;
	MPxContext* makeObj() override;
	MStatus		appendSyntax() override;
	static void*		creator();

protected:
    helixContext*		fHelixContext;

};

helixContextCmd::helixContextCmd() {}

MPxContext* helixContextCmd::makeObj()
//
// Description
//    When the context command is executed in maya, this method
//    be used to create a context.
//
{
	fHelixContext = new helixContext();
	return fHelixContext;
}

void* helixContextCmd::creator()
//
// Description
//    This method creates the context command.
//
{
	return new helixContextCmd;
}


MStatus helixContextCmd::doEditFlags()
{
	MStatus status = MS::kSuccess;
	
	MArgParser argData = parser();
	
	if (argData.isFlagSet(kNumberCVsFlag)) {
		unsigned numCVs;
		status = argData.getFlagArgument(kNumberCVsFlag, 0, numCVs);
		if (!status) {
			status.perror("numCVs flag parsing failed.");
			return status;
		}
		fHelixContext->setNumCVs(numCVs);
	}

	if (argData.isFlagSet(kUpsideDownFlag)) {
		bool upsideDown;
		status = argData.getFlagArgument(kUpsideDownFlag, 0, upsideDown);
		if (!status) {
			status.perror("upsideDown flag parsing failed.");
			return status;
		}
		fHelixContext->setUpsideDown(upsideDown);
	}
	
	return MS::kSuccess;
}

MStatus helixContextCmd::doQueryFlags()
{
	MArgParser argData = parser();
	
	if (argData.isFlagSet(kNumberCVsFlag)) {
		setResult((int) fHelixContext->numCVs());
	}
	if (argData.isFlagSet(kUpsideDownFlag)) {
		setResult(fHelixContext->upsideDown());
	}
	
	return MS::kSuccess;
}

MStatus helixContextCmd::appendSyntax()
{
	MSyntax mySyntax = syntax();
	
	if (MS::kSuccess != mySyntax.addFlag(kNumberCVsFlag, kNumberCVsFlagLong,
										 MSyntax::kUnsigned)) {
		return MS::kFailure;
	}
	if (MS::kSuccess != 
		mySyntax.addFlag(kUpsideDownFlag, kUpsideDownFlagLong,
						 MSyntax::kBoolean)) {
		return MS::kFailure;
	}

	return MS::kSuccess;
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
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

	// Register the context creation command and the tool command 
	// that the helixContext will use.
	// 
	status = plugin.registerContextCommand("helixToolContext",
										   helixContextCmd::creator,
										   "helixToolCmd",
										   helixTool::creator,
										   helixTool::newSyntax);
	if (!status) {
		status.perror("registerContextCommand");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus status;
	MFnPlugin plugin( obj );

	// Deregister the tool command and the context creation command
	//
	status = plugin.deregisterContextCommand( "helixToolContext",
											  "helixToolCmd" );
	if (!status) {
		status.perror("deregisterContextCommand");
		return status;
	}

	return status;
}
