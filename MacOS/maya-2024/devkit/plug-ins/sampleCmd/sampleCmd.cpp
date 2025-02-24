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

///////////////////////////////////////////////////////////////////////
// DESCRIPTION:
// 
// Produces the MEL command "sampleCmd".
//
// This example demonstrates how to sample shading group/node using
// MRenderUtil::sampleShadingNetwork().
//
// The command takes lists of shading info such as pointCamera and UVs, samples
// a given shading group/node, and returns the result colors and transparencies.
// Lighting will be calculated if a shading group is sampled. Shadows can be
// calculated as well if the -shadow flag is specified. For example: 
//
//	sampleCmd file1.outColor 4 -uvs 0 0 1 0 1 1 0 1;
//	sampleCmd creater1.outColor 4 -refPoints 0 0 0 1 0 0 1 0 1 0 0 1;
//
// Note that sampleCmd -h provides more details on how to use the command.
//
////////////////////////////////////////////////////////////////////////

#include <maya/MIOStream.h>
#include <stdio.h>
#include <stdlib.h>

#include <maya/MPxCommand.h>
#include <maya/MFnPlugin.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>

#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MFloatMatrix.h>

#include <maya/MRenderUtil.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>

#include <maya/MVector.h>
#include <maya/MPoint.h>

#include <math.h>

class sampleCmd : public MPxCommand
{
	MStatus			printErr();
public:
					sampleCmd() {};
				~sampleCmd() override; 

	MStatus			doIt( const MArgList& args ) override;
	static void*	creator();
};

sampleCmd::~sampleCmd() {}

void* sampleCmd::creator()
{
	return new sampleCmd();
}

MStatus sampleCmd::printErr()
{
	displayError( "Usage: sampleCmd [-shadow|-reuse] <shadingEngine|shadingNode.plug> <numSamples>\n"
		"  [-points p0.x p0.y p0.z p1.x p1.y p1.z ...]\n"
		"  [-refPoints rp0.x rp0.y rp0.z rp1.x rp1.y rp1.z ...]\n"
		"  [-uvs u0 v0 u1 v1 ...]\n"
		"  [-normals n0.x n0.y n0.z n1.x n1.y n1.z ...]\n"
		"  [-tangentUs tu0.x tu0.y tu0.z tu1.x tu1.y tu1.z ...]\n"
		"  [-tangentVs tv0.x tv0.y tv0.z tv1.x tv1.y tv1.z ...]\n"
		"  [-filterSizes f0 f1 ...]\n" 
		"Result:\n" 
		"  clr0.r clr0.g clr0.b clr1.r clr1.g clr1.b ... transp0.r transp0.g transp0.b transp1.r transp1.g transp1.b ...\n" 
	);
	return MS::kFailure;
}

MStatus sampleCmd::doIt( const MArgList& args )
{
	MStatus stat;

	unsigned int i;
	bool shadow = 0;
	bool reuse = 0;

	for ( i = 0; i < args.length(); i++ )
	{
		if ( args.asString(i) == MString("-shadow") || 
			args.asString(i) == MString("-s") )
			shadow = 1;
		else if ( args.asString(i) == MString("-reuse") || 
			args.asString(i) == MString("-r") )
			reuse = 1;
		else
			break;
	}
// fprintf( stderr, "length: %d\n", args.length() - i );
	if ( args.length() - i < 4 )
		return printErr();

	if ( reuse && !shadow )	// can only reuse if shadow is turned on
		reuse = 0;

	MString node = args.asString( i );
	int numSamples = args.asInt( i+1 );

	fprintf( stderr, "num samples: %d\n", numSamples );

	MFloatArray uCoords, vCoords, filterSizes;
	MFloatPointArray points, refPoints;
	MFloatVectorArray normals, tanUs, tanVs;

	int j;
	MVector vector;
	MPoint point;
	i+=2;
	for ( ; i < args.length(); i++ )
	{
		if ( args.asString( i ) == MString( "-points" ) )
		{
			i++;
			for ( j = 0; j < numSamples; j++ )
			{
				if ( MS::kSuccess != args.get( i, point ) )
					return printErr();
				points.append( MFloatPoint((float)point.x, (float)point.y, (float)point.z) );
			}
		}
		else if ( args.asString( i ) == MString( "-refPoints" ) )
		{
			i++;
			for ( j = 0; j < numSamples; j++ )
			{
				if ( MS::kSuccess != args.get( i, point ) )
					return printErr();
				refPoints.append( MFloatPoint((float)point.x, (float)point.y, (float)point.z) );
			}
		}
		else if ( args.asString( i ) == MString( "-normals" ) )
		{
			i++;
			for ( j = 0; j < numSamples; j++ )
			{
				if ( MS::kSuccess != args.get( i, vector ) )
					return printErr();
				normals.append( MFloatVector((float)vector.x, (float)vector.y, (float)vector.z) );
			}
		}
		else if ( args.asString( i ) == MString( "-tangentUs" ) )
		{
			i++;
			for ( j = 0; j < numSamples; j++ )
			{
				if ( MS::kSuccess != args.get( i, vector ) )
					return printErr();
				tanUs.append( MFloatVector((float)vector.x, (float)vector.y, (float)vector.z) );
			}
		}
		else if ( args.asString( i ) == MString( "-tangentVs" ) )
		{
			i++;
			for ( j = 0; j < numSamples; j++ )
			{
				if ( MS::kSuccess != args.get( i, vector ) )
					return printErr();
				tanVs.append( MFloatVector((float)vector.x, (float)vector.y, (float)vector.z) );
			}
		}
		else if ( args.asString( i ) == MString( "-uvs" ) )
		{
			i++;
			for ( j = 0; j < numSamples; j++ )
			{
				double d = args.asDouble( i, &stat );
				if ( stat != MS::kSuccess )
					return printErr();
				uCoords.append( (float)d );
				i++;
				d = args.asDouble( i, &stat );
				if ( stat != MS::kSuccess )
					return printErr();
				vCoords.append( (float)d );
				i++;
			}
		}
		else if ( args.asString( i ) == MString( "-filterSizes" ) )
		{
			i++;
			for ( j = 0; j < numSamples; j++ )
			{
				double d = args.asDouble( i, &stat );
				if ( stat != MS::kSuccess )
					return printErr();
				filterSizes.append( (float)d );
				i++;
			}
		}
		else 
		{
			fprintf( stderr, "*** ERROR: Bad args: %s\n", args.asString( i ).asChar() );
			return printErr();
		}

	}	// for i < length()

	// get current camera

	MDagPath cameraPath;
	M3dView::active3dView().getCamera( cameraPath );
	MMatrix mat = cameraPath.inclusiveMatrix();
	MFloatMatrix cameraMat( mat.matrix );


	MFloatVectorArray colors, transps;
	if ( MS::kSuccess == MRenderUtil::sampleShadingNetwork( 
			node, 
			numSamples,
			shadow,
			reuse,

			cameraMat,

			(points.length()>0) 	? &points : NULL,
			(uCoords.length()>0) 	? &uCoords : NULL,
			(vCoords.length()>0) 	? &vCoords : NULL,
			(normals.length()>0) 	? &normals : NULL,
			(refPoints.length()>0) 	? &refPoints : NULL,
			(tanUs.length()>0) 		? &tanUs : NULL,
			(tanVs.length()>0) 		? &tanVs : NULL,
			(filterSizes.length()>0) ? &filterSizes : NULL,

			colors,
			transps ) )
	{
		fprintf( stderr, "%u points sampled...\n", colors.length() );
		for ( i = 0; i < colors.length(); i++ )
		{
			appendToResult( (double) colors[i].x );
			appendToResult( (double) colors[i].y );
			appendToResult( (double) colors[i].z );
		}
		for ( i = 0; i < transps.length(); i++ )
		{
			appendToResult( (double) transps[i].x );
			appendToResult( (double) transps[i].y );
			appendToResult( (double) transps[i].z );
		}
	}
	else
	{
		displayError( node + MString(" is not a shading engine!  Specify node.attr or shading group node." ) );
	}

	return MS::kSuccess;
}


//
// The following routines are used to register/unregister
// the command we are creating within Maya
//
MStatus initializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerCommand( "sampleCmd", sampleCmd::creator );
	if (!status) {
		status.perror("registerCommand");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterCommand( "sampleCmd" );
	if (!status) {
		status.perror("deregisterCommand");
		return status;
	}

	return status;
}
