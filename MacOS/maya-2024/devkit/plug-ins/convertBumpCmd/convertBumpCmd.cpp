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

//
// ---------------------------------------------------------------------------
//
// Description:
// ------------
//		This plug-in command can be used to convert a bump file texture
// from the a grey-scale height field format (used by Maya) to a normal map
// format that is typically used for real-time, hardware-based rendering.
//
//		This code also demonstrates how to use the MImage class to load,
// manipulate and save image files on disk. 
// (Note that the MImage class is new as of Maya 4.0.1, but that the saving
// and filtering capability was only introduced in Maya4.5)
//
//
// Usage:
// ------
//		To test this plug-in, first compile and load it using the plug-in 
// manager, then type the following in the script editor:
//
// convertBump "C:/bump.tga" "C:/bump_norm.tga" "tga" 1.0
//
//		This would convert the input texture (c:/bump.tga) into
// an output normal map (c:/bump_norm.tga) using the default
// bumpScale ratio. The bumpScale parameter can be used to increase or
// decrease the bumpiness of the resulting normal map.
//
//		See the documentation of MImage::saveToFile() for a complete
// list of the available file formats supported.
//

#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>
#include <maya/MFnPlugin.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MPoint.h>
#include <maya/MImage.h>

#include <maya/MIOStream.h>

#define IFFCHECKERR(stat, call) \
if (!stat) { \
	MString string = reader.errorString(); \
    string += " in method "; \
	string += #call; \
    displayError (string); \
	return MS::kFailure; \
}

class convertBump : public MPxCommand
{
public:
                convertBump();
        ~convertBump() override;

    MStatus     doIt ( const MArgList& args ) override;
    bool        isUndoable() const override;

    static      void* creator();
};

convertBump::convertBump()
{
}

convertBump::~convertBump() {}

void* convertBump::creator()
{
    return new convertBump;
}

bool convertBump::isUndoable() const
{
    return false;
}

MString itoa (int n)
{
	char buffer [256];
	sprintf (buffer, "%d", n);
	return MString (buffer);
}

MStatus convertBump::doIt( const MArgList& args )
{
	// Verify that the required conversion exists before doing anything else.
	if (!MImage::filterExists(MImage::kHeightFieldBumpFormat, MImage::kNormalMapBumpFormat))
	{
		displayError ("Fatal Error! The required filter (kHeightFieldBumpFormat -> kNormalMapBumpFormat) isn't supported!");
		return MS::kFailure;
	}

	if (args.length () < 2 || args.length () > 4) 
	{
		displayError ("Syntax: convertBump inputFile outputFile [outputFormat [bumpScale]]");
		displayError ("(eg: convertBump \"C:/bump.tga\" \"C:/bump_norm.tga\" \"tga\" 1.0)");
		return MS::kFailure;
	}

	double bumpScale = 1.0;

	MString inputFilename, outputFilename;
	MString outputFormat = "iff";

	args.get(0, inputFilename);
	args.get(1, outputFilename);

	if (args.length() > 2)
		args.get(2, outputFormat);

	// Get the desired bumpscale, if it is provided as an argument.
	if (args.length() == 4)
	{
		MString lastArg;
		args.get (3, bumpScale);
	}


	MImage image;
	MStatus stat = image.readFromFile(inputFilename);
	if (!stat)
	{
		displayError ("Unable to open input file \"" + inputFilename + "\".");
		return MS::kFailure;
	}

	// Get the dimension of the texture.
	unsigned int width, height;
	stat = image.getSize(width, height);
	if (!stat)
	{
		displayError ("Unable to get size.");
		return MS::kFailure;
	}

	// Attempt to convert from height field to normal map.
	stat = image.filter(MImage::kHeightFieldBumpFormat, MImage::kNormalMapBumpFormat, bumpScale);
	if (!stat)
	{
		displayError ("Unable to apply the filter from height field to normal map bump format.");
		return MS::kFailure;
	}

	// Save back to disk.
	stat = image.writeToFile(outputFilename, outputFormat);
	if (!stat)
	{
		displayError ("Unable to write to output file \"" + outputFilename + "\" using output format " + outputFormat + 
			". (read-only? disk full? invalid path?)");
		return MS::kFailure;
	}

    return MS::kSuccess;
}

MStatus initializePlugin( MObject obj )
{
	MStatus status;
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.0.1", "Any");
    status = plugin.registerCommand( "convertBump", convertBump::creator );

	if (!status)
		status.perror("registerCommand");

    return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus status;
    MFnPlugin plugin( obj );
    status = plugin.deregisterCommand( "convertBump" );

	if (!status)
		status.perror("deregisterCommand");

    return status;
}
