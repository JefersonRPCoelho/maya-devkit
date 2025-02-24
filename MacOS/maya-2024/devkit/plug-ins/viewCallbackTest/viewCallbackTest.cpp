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
// Produces the command "viewCallbackTest". 
//
// This plug-in installs the pre and post rendering callbacks of MUiMessage.
// As a simple demonstration, a modelling view can be inverted or shaded based on depth.
// In the depth shaded mode, the closer objects are lighter in color. Below is some
// sample MEL for using this plug-in once it is loaded.
//
//	// Invert mode
//	viewCallbackTest -bo "invert" `getPanel -withFocus`;
//	// Depth mode
//	viewCallbackTest -bo "showDepth" `getPanel -withFocus`;
//
// Note that the screen does not refresh right after the plug-in execution.
//
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MEventMessage.h>
#include <maya/MUiMessage.h>
#include <maya/MFnPlugin.h>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>

#include <maya/M3dView.h>
#include <maya/MPoint.h>
#include <maya/MImage.h>

#include <stdio.h>

// Static pointer to the current refreshCompute per panel so we can delete
// it if the plug-in is unloaded.
//
class refreshCompute;
static refreshCompute* currentRefreshCompute[4] = { 0, 0, 0, 0 };

// Possible buffer operations supported
enum MbufferOperation {
	kInvertColorBuffer,
	kDrawDepthBuffer
};

///////////////////////////////////////////////////
//
// Command class declaration
//
///////////////////////////////////////////////////
class viewCallbackTest : public MPxCommand
{
public:
	viewCallbackTest();
	                ~viewCallbackTest() override; 

	MStatus                 doIt( const MArgList& args ) override;

	static MSyntax			newSyntax();
	static void*			creator();

private:
	MStatus                 parseArgs( const MArgList& args );

	// Name of panel to monitor
	MString					mPanelName;
	MbufferOperation		mBufferOperation;
};

///////////////////////////////////////////////////
//
// Refresh computation class implementation
//
///////////////////////////////////////////////////
class refreshCompute
{
public:
	refreshCompute(const MString &panelName, MbufferOperation bufferOperation);
	~refreshCompute();

	const MString &panelName() const { return mPanelName; }
	void setPanelName(const MString &panelName) { mPanelName = panelName; }

	MbufferOperation bufferOperation() const { return mBufferOperation; }
	void setBufferOperation(const MbufferOperation operation) { mBufferOperation = operation; }

	void clearCallbacks();

protected:
	static void				deleteCB(const MString& panelName, void * data);
	static void				preRenderCB(const MString& panelName, void * data);
	static void				postRenderCB(const MString& panelName, void * data);

	MCallbackId				mDeleteId;
	MCallbackId				mPreRenderId;
	MCallbackId				mPostRenderId;

	MString					mPanelName;
	MbufferOperation		mBufferOperation;
};


///////////////////////////////////////////////////
//
// Command class implementation
//
///////////////////////////////////////////////////

// Constructor
//
viewCallbackTest::viewCallbackTest()
{
	mPanelName = MString("");
	mBufferOperation = kInvertColorBuffer;
}

// Destructor
//
viewCallbackTest::~viewCallbackTest()
{
	// Do nothing
}

// creator
//
void* viewCallbackTest::creator()
{
	return (void *) (new viewCallbackTest);
}

// newSyntax
//
// Buffer operation = -bo/-bufferOperation <string> = (invert | showDepth)
const char *bufferOperationShortName = "-bo";
const char *bufferOperationLongName = "-bufferOperation";
#define _NUMBER_BUFFER_OPERATIONS_ 2
const MString bufferOperationStrings[_NUMBER_BUFFER_OPERATIONS_ ] = 
		{ MString("invert"), MString("showDepth") };
const MbufferOperation bufferOperations[_NUMBER_BUFFER_OPERATIONS_] = 
		{ kInvertColorBuffer, kDrawDepthBuffer };

MSyntax viewCallbackTest::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag(bufferOperationShortName, bufferOperationLongName, MSyntax::kString);

	// Name of model panel to monitor
	//
	syntax.addArg(MSyntax::kString);

	return syntax;
}

// parseArgs
//
MStatus viewCallbackTest::parseArgs(const MArgList& args)
{
	MStatus			status;
	MArgDatabase	argData(syntax(), args);

	// Buffer operation argument variables
	mBufferOperation = kInvertColorBuffer;
	MString operationString;

	MString     	arg;
	for ( unsigned int i = 0; i < args.length(); i++ ) 
	{
		arg = args.asString( i, &status );
		if (!status)              
			continue;

		if ( arg == MString(bufferOperationShortName) || arg == MString(bufferOperationLongName) ) 
		{
			if (i == args.length()-1) {
				arg += ": must specify a buffer operation.";
				displayError(arg);
				return MS::kFailure;
			}
			i++;
			args.get(i, operationString );

			bool validOperation = false;
			for (unsigned int k=0; k<_NUMBER_BUFFER_OPERATIONS_; k++)
			{
				if (bufferOperationStrings[i] == operationString)
				{
					mBufferOperation = bufferOperations[k];
					validOperation = true;
				}
			}
			if (!validOperation)
				status.perror("Invalid operation specified. Using invert by default.");
		}
	}

	// Read off the panel name
	status = argData.getCommandArgument(0, mPanelName);
	if (!status)
	{
		status.perror("No panel name specified as command argument");
		return status;
	}
	return MS::kSuccess;
}

// doIt
//
MStatus viewCallbackTest::doIt(const MArgList& args)
{
	MStatus status;

	status = parseArgs(args);
	if (!status)
	{
		return status;
	}

	try
	{
		// Only allow one computation per panel at this time.
		refreshCompute *foundComputePtr = 0;
		unsigned int i;
		for (i=0; i<4; i++)
		{
			if (currentRefreshCompute[i] && 
				(currentRefreshCompute[i])->panelName() == mPanelName)
			{
				foundComputePtr = currentRefreshCompute[i];
			}
		}

		// If alread exists, just change the operator if it differs.
		if (foundComputePtr)
		{
			foundComputePtr->setBufferOperation( mBufferOperation);
		}
		else
		{
			for (i=0; i<4; i++)
			{
				if (!currentRefreshCompute[i])
				{
					currentRefreshCompute[i] = new refreshCompute(mPanelName, mBufferOperation);
					break;
				}
			}
		}
	}
	catch(MStatus status)
	{
		return status;
	}
	catch(...)
	{
		throw;
	}

	return status;
}

///////////////////////////////////////////////////
//
// refreshCompute implementation
//
///////////////////////////////////////////////////
refreshCompute::refreshCompute(const MString &panelName, 
							   MbufferOperation postBufferOperation)
{
	MStatus status;

	// Set panel name and operator for post rendering
	mPanelName = panelName;
	mBufferOperation = postBufferOperation;

	// Add the callbacks
	//
	mDeleteId
		= MUiMessage::add3dViewDestroyMsgCallback(panelName,
										   &refreshCompute::deleteCB,
										   (void *) this, &status);
	if (mDeleteId == 0)
		status.perror(MString("Could not attach view deletion callback to panel ") +
					  panelName);

	mPreRenderId
		= MUiMessage::add3dViewPreRenderMsgCallback(panelName,
										  &refreshCompute::preRenderCB,
										  (void *) this, &status);
	if (mPreRenderId == 0)
		status.perror(MString("Could not attach view prerender callback to panel ") +
				panelName);

	mPostRenderId
		= MUiMessage::add3dViewPostRenderMsgCallback(panelName,
										   &refreshCompute::postRenderCB,
										   (void *) this, &status);
	if (mPostRenderId == 0)
		status.perror(MString("Could not attach view postrender callback to panel ") +
					  panelName);
}

// Clear all callbacks for this compute
void refreshCompute::clearCallbacks()
{	
	if (mDeleteId)
		MMessage::removeCallback(mDeleteId);

	if (mPreRenderId)
		MMessage::removeCallback(mPreRenderId);

	if (mPostRenderId)
		MMessage::removeCallback(mPostRenderId);
}

refreshCompute::~refreshCompute()
{
	clearCallbacks();

	// Reset any global pointer pointing to this compute
	for (unsigned int i=0; i<4; i++)
	{
		if (currentRefreshCompute[i] && 
			(currentRefreshCompute[i])->panelName() == mPanelName)
		{
			currentRefreshCompute[i] = 0;
		}
	}
}

// Delete callback
//
void refreshCompute::deleteCB(const MString& panelName, void * data)
{
	refreshCompute *pf = (refreshCompute *) data;

	cout<<"In delete view callback for view "<<panelName.asChar()
		<<". Remove all callbacks."<<endl;

	// Delete callback.
	delete pf;
}

// Pre-render callback
//
void refreshCompute::preRenderCB(const MString& panelName, void * data)
{
	refreshCompute *thisCompute = (refreshCompute *) data;
	if (!thisCompute)
		return;

	M3dView view;
	MStatus status = M3dView::getM3dViewFromModelPanel(panelName, view);
	if (status != MS::kSuccess)
		return;

	int width = 0, height = 0;
	width = view.portWidth( &status );
	if (status != MS::kSuccess || (width < 2))
		return;
	height = view.portHeight( &status );
	if (status != MS::kSuccess || (height < 2))
		return;

	unsigned int vx,vy,vwidth,vheight;
	vx = 0;
	vy = 0;
	vwidth = width / 2;
	vheight = height / 2;
	status = view.pushViewport (vx, vy, vwidth, vheight);

	if (thisCompute->mBufferOperation != kDrawDepthBuffer)
	{
		M3dView view;
		MStatus status = M3dView::getM3dViewFromModelPanel(panelName, view);
		MPoint origin;

		_OPENMAYA_DEPRECATION_PUSH_AND_DISABLE_WARNING
		status = view.drawText( MString("Pre render callback: ") + panelName, origin );
		_OPENMAYA_POP_WARNING
	}

	view.popViewport();
}

// Post-render callback
//
void refreshCompute::postRenderCB(const MString& panelName, void * data)
{	
	refreshCompute *thisCompute = (refreshCompute *) data;
	if (!thisCompute)
		return;

	// Get the view if any for the panel
	M3dView view;
	MStatus status = M3dView::getM3dViewFromModelPanel(panelName, view);
	if (status != MS::kSuccess)
		return;

	if (thisCompute->mBufferOperation == kDrawDepthBuffer)
	{
		int width = 0, height = 0;
		width = view.portWidth( &status ) / 2 ;
		if (status != MS::kSuccess || (width < 2))
			return;
		height = view.portHeight( &status ) / 2 ;
		if (status != MS::kSuccess || (height < 2))
			return;

		unsigned int numPixels = width * height;

		float *depthPixels = new float[numPixels];
		if (!depthPixels)
			return;

		unsigned char *colorPixels = new unsigned char[numPixels * 4];
		if (!colorPixels)
		{
			delete [] depthPixels;
			delete [] colorPixels;
			return;
		}

		_OPENMAYA_DEPRECATION_PUSH_AND_DISABLE_WARNING
		// Read into a float buffer
		status = view.readDepthMap( 0,0, width, height, (unsigned char *)depthPixels, M3dView::kDepth_Float );
		_OPENMAYA_POP_WARNING

		if (status != MS::kSuccess)
		{
			delete [] depthPixels;
			delete [] colorPixels;
			return;
		}

		// Find depth range and remap normalized depth range (-1 to 1) into 0...255
		// for color.
		float *dPtr = depthPixels;
		unsigned int i = 0;

		float zmin = 100.0f; // *dPtr;
		float zmax = -100.0f; // *dPtr;
		for(i=0; i<numPixels; i++)
		{
			float val = *dPtr; // * 2.0f - 1.0f;
			if(val < zmin) {
				zmin = *dPtr;
			}
			if(val > zmax) {
				zmax = *dPtr;
			}
			dPtr++;
		}
		float zrange = zmax - zmin;
		//printf("depth values = (%g, %g). Range = %g\n", zmin, zmax, zrange);

		unsigned char *cPtr = colorPixels;

		dPtr = depthPixels;
		for(i=0; i < numPixels; i++)
		{
			float val = *dPtr; // * 2.0f - 1.0f;
			//unsigned char depth = (unsigned char)(255.0f * (( (*dPtr)-zmin) / zrange) + zmin );
			unsigned char depth = (unsigned char)(255.0f * (( (val)-zmin) / zrange) );
			//unsigned char depth = (unsigned char)(255.0f * val);
			*cPtr = depth; cPtr++;
			*cPtr = depth; cPtr++;
			*cPtr = depth; cPtr++;
			*cPtr = 0xff;   
			cPtr++;
			dPtr++;
		}

		MImage image;
		image.setPixels( colorPixels, width, height );

		// Uncomment next line to test writing buffer to file.
		//image.writeToFile( "C:\\temp\\dumpDepth.iff" );

		_OPENMAYA_DEPRECATION_PUSH_AND_DISABLE_WARNING
		// Write all pixels back. The origin of the image (lower left)
		// is used 
		status = view.writeColorBuffer( image, 5, 5 );
		_OPENMAYA_POP_WARNING

		if (depthPixels)
			delete [] depthPixels;
		if (colorPixels)
			delete [] colorPixels;
	}

	// Do a simple color invert operation on all pixels
	//
	else if (thisCompute->mBufferOperation == kInvertColorBuffer)
	{
		// Optional to read as RGBA. Note that this will be slower
		// since we must swizzle the bytes around from the default
		// BGRA format.
		bool readAsRGBA = true;

		// Read the RGB values from the color buffer
		MImage image;
		_OPENMAYA_DEPRECATION_PUSH_AND_DISABLE_WARNING
		status = view.readColorBuffer( image, readAsRGBA );
		_OPENMAYA_POP_WARNING
		if (status != MS::kSuccess)
			return;

		_OPENMAYA_DEPRECATION_PUSH_AND_DISABLE_WARNING
		status = view.writeColorBuffer( image, 5, 5 );
		_OPENMAYA_POP_WARNING

		unsigned char *pixelPtr = (unsigned char*)image.pixels();
		if (pixelPtr)
		{
			unsigned int width, height;
			image.getSize( width, height );

			MImage image2;
			image2.create( width, height );
			unsigned char *pixelPtr2 = (unsigned char*)image2.pixels();

			unsigned int numPixels = width * height;
			for (unsigned int i=0; i < numPixels; i++)
			{
				*pixelPtr2 = (255 - *pixelPtr);	
				pixelPtr2++; pixelPtr++;
				*pixelPtr2 = (255 - *pixelPtr);	
				pixelPtr2++; pixelPtr++;
				*pixelPtr2 = (255 - *pixelPtr);	
				pixelPtr2++; pixelPtr++;
				*pixelPtr2 = 255;	
				pixelPtr2++; pixelPtr++;
			}

			_OPENMAYA_DEPRECATION_PUSH_AND_DISABLE_WARNING
			// Write all pixels back. The origin of the image (lower left)
			// is used 
			status = view.writeColorBuffer( image2, 5, short(5+height/2) );
			_OPENMAYA_POP_WARNING
		}
	}
}

///////////////////////////////////////////////////
//
// Plug-in functions
//
///////////////////////////////////////////////////

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "6.5", "Any");

	// Register the command so we can actually do some work
	//
	status = plugin.registerCommand("viewCallbackTest",
									viewCallbackTest::creator,
									viewCallbackTest::newSyntax);

	if (!status)
	{
		status.perror("registerCommand");
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	// Remove all computation class + callbacks
	for (unsigned int i=0; i<4; i++)
	{
		delete currentRefreshCompute[i];
		currentRefreshCompute[i] = 0;
	}

	// Deregister the command
	//
	status = plugin.deregisterCommand("viewCallbackTest");

	if (!status)
	{
		status.perror("deregisterCommand");
	}

	return status;
}

