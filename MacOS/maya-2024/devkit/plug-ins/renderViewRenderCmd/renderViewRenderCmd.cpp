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
// Produces the MEL command "renderViewRender".
//
// This example demonstrates how to render a full image to the Render View
// window using the MRenderView class. The command takes no arguments.
// It renders a 640x480 image tiled with a red and white circular pattern
// to the Render View. 
//
////////////////////////////////////////////////////////////////////////

#include <maya/MSimple.h>
#include <maya/MIOStream.h>
#include <maya/MRenderView.h>
#include <maya/M3dView.h>
#include <math.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

//
//	renderViewRender command declaration
//
class renderViewRender : public MPxCommand 
{							
public:					
	renderViewRender() {};
	~renderViewRender() override {};

	MStatus	doIt ( const MArgList& ) override;
	
	static void*	creator();
	
	static MSyntax	newSyntax();
	MStatus parseSyntax (MArgDatabase &argData);

	static const char * cmdName;

private:
	bool doNotClearBackground;				

};													

static const char * kDoNotClearBackground		= "-b";
static const char * kDoNotClearBackgroundLong	= "-background";

const char * renderViewRender::cmdName = "renderViewRender";

void* renderViewRender::creator()					
{													
	return new renderViewRender;					
}													

MSyntax renderViewRender::newSyntax()
{
	MStatus status;
	MSyntax syntax;
	syntax.addFlag( kDoNotClearBackground, kDoNotClearBackgroundLong );
	CHECK_MSTATUS_AND_RETURN(status, syntax);
	return syntax;
}

//
// Description:
//		Read the values of the additionnal flags for this command.
//
MStatus renderViewRender::parseSyntax (MArgDatabase &argData)
{
	// Get the flag values, otherwise the default values are used.
	doNotClearBackground = argData.isFlagSet( kDoNotClearBackground );
	
	return MS::kSuccess;
}

//
// Description:
//		register the command
//
MStatus initializePlugin( MObject obj )			
{															
	MFnPlugin	plugin( obj, PLUGIN_COMPANY, "4.5" );	
	MStatus		stat;										
	stat = plugin.registerCommand(	renderViewRender::cmdName,
									renderViewRender::creator,
									renderViewRender::newSyntax);	
	if ( !stat )												
		stat.perror( "registerCommand" );							
	return stat;												
}																

//
// Description:
//		unregister the command
//
MStatus uninitializePlugin( MObject obj )						
{																
	MFnPlugin	plugin( obj );									
	MStatus		stat;											
	stat = plugin.deregisterCommand( renderViewRender::cmdName );				
	if ( !stat )									
		stat.perror( "deregisterCommand" );			
	return stat;									
}

RV_PIXEL evaluate(int x, int y)
//
//	Description:
//		Generates a simple procedural circular pattern to be sent to the 
//		Render View.
//
//	Arguments:
//		x, y - coordinates in the current tile (the pattern is centred 
//			   around (0,0) ).
//
//	Return Value:
//		An RV_PIXEL structure containing the colour of pixel (x,y).
//
{
	unsigned int distance = (unsigned int) sqrt(double((x*x) + (y*y)));

	RV_PIXEL pixel;
	// Always fully red.
	pixel.r = 255.0f;		
	// Green and blue varies according to the distance.
	pixel.g = pixel.b = 155.0f + (distance % 20) * 5;
	pixel.a = 255.0f;

	return pixel;
}

MStatus renderViewRender::doIt( const MArgList& args )
//
//	Description:
//		Implements the MEL renderViewRender command.  This command
//		Draws a 640x480 tiled pattern of red and white circles into Maya's
//		Render View window.  
//
//	Arguments:
//		args - The argument list that was passed to the command from MEL.
//				-background/-b renders the pattern without clearing the Render View
//
//	Return Value:
//		MS::kSuccess - command succeeded
//		MS::kFailure - command failed (returning this value will cause the 
//                     MEL script that is being run to terminate unless the
//                     error is caught using a "catch" statement.
//
{
	MStatus stat = MS::kSuccess;

	// Check if the render view exists. It should always exist, unless
	// Maya is running in batch mode.
	//
	if (!MRenderView::doesRenderEditorExist())
	{
		setResult( "Cannot renderViewRender in batch render mode. "
				   "Please run in interactive mode, "
				   "so that the render editor exists." );
		return MS::kFailure;
	}
	else
	{
		cout<<"Past doesRenderEditorExist()"<<endl;
	}
	
	// get optional flags
	MArgDatabase	argData( syntax(), args );
	parseSyntax( argData );

	// Pick a camera, and tell the Render View that we will be rendering
	// from its point of view.  Just use the camera for the active 
	// modelling view.  
	//
	M3dView curView = M3dView::active3dView();
	MDagPath camDagPath;
	curView.getCamera( camDagPath );
	cout<<"Rendering camera"<<camDagPath.fullPathName().asChar()<<endl;
	
	if( MRenderView::setCurrentCamera( camDagPath ) != MS::kSuccess )
	{
		setResult( "renderViewRender: error occurred in setCurrentCamera." );
		return MS::kFailure;
	}

	// We'll render a 640x480 image.  Tell the Render View to get ready
	// to receive 640x480 pixels of data.
	//
	unsigned int image_width = 640, image_height = 480;
	if (MRenderView::startRender( image_width, image_height, doNotClearBackground) != MS::kSuccess)
	{
		setResult( "renderViewRender: error occured in startRender." );
		return MS::kFailure;
	}

	// The image will be composed of tiles consisting of circular red and
	// white patterns.
	//
	unsigned int num_side_tiles = 8;
	unsigned int average_tiles_width = image_width / num_side_tiles;
	unsigned int average_tiles_height = image_height / num_side_tiles;
	
	// Draw each tile
	//
	for (unsigned int tile_y = 0; tile_y < num_side_tiles; tile_y++)
	{
		for (unsigned int tile_x = 0; tile_x < num_side_tiles; tile_x++)
		{
			// Find the min/max width/height.
			int min_x = tile_x * average_tiles_width;
			int max_x;

			// If this is the last tile in width, adjust the max position 
			// so that the entire width is covered.
			//
			if ((tile_x+1) == num_side_tiles)
				max_x = image_width-1;
			else
				max_x = (tile_x + 1) * average_tiles_width - 1;
					
			int min_y = tile_y * average_tiles_height;
			int max_y;

			// If this is the last tile in height, adjust the max position 
			// so that the entire height is covered.
			//
			if ((tile_y+1) == num_side_tiles)
				max_y = image_height-1;
			else
				max_y = (tile_y + 1) * average_tiles_height - 1;
			
			unsigned int tile_width = max_x - min_x + 1; 
			unsigned int tile_height = max_y - min_y + 1;

			// Fill up the pixel array with some the pattern, which is 
			// generated by the 'evaluate' function.  The Render View
			// accepts floating point pixel values only.
			//
			RV_PIXEL* pixels = new RV_PIXEL[tile_width * tile_height];
			unsigned int index = 0;
			for (unsigned int j = 0; j < tile_height; j++ )
			{
				for (unsigned int i = 0; i < tile_width; i++)
				{
					pixels[index] = evaluate(i - (tile_width / 2), 
											 j - (tile_height / 2));
					index++;
				}
			}

			// Send the data to the render view.
			//
			if (MRenderView::updatePixels(min_x, max_x, min_y, max_y, pixels) != MS::kSuccess)
			{
				setResult( "renderViewRender: error occured in updatePixels." );
				delete [] pixels;
				return MS::kFailure;
			}

			delete [] pixels;

			// Force the Render View to refresh the display of the affected 
			// region.
			//
			if (MRenderView::refresh(min_x, max_x, min_y, max_y) != MS::kSuccess)
			{
				setResult( "renderViewRender: error occured in refresh." );
				return MS::kFailure;
			}
		}
	}

	// Inform the Render View that we have completed rendering the entire image.
	//
	if (MRenderView::endRender() != MS::kSuccess)
	{
		setResult( "renderViewRender: error occured in endRender." );
		return MS::kFailure;
	}

	setResult( "renderViewRender completed." );
	return stat;
}




