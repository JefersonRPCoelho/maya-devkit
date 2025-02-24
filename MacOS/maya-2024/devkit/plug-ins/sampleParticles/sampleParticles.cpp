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
// Produces the MEL command "sampleParticles".
//
// This example demonstrates how to sample shading groups or nodes using 
// MRenderUtil::sampleShadingNetwork() to assign colors to a particle object.
// 
// The command takes lists of shading info such as pointCamera and UVs, samples
// a given shading group or node, then assigns the sampled colors and transparencies
// to a grid of particles using the emit command. Lighting will be calculated if
// a shading group is sampled. Shadows can be calculated if the -shadow flag is specified.
//
// Note that sampleParticles -h provides more details on how to use the command.
//
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
#include <math.h>

#ifndef M_PI
#define	M_PI		3.14159265358979323846	/* pi */
#endif

class sampleParticles : public MPxCommand
{
public:
					sampleParticles() {};
				~sampleParticles() override; 

	MStatus			doIt( const MArgList& args ) override;
	static void*	creator();
};

sampleParticles::~sampleParticles() {}

void* sampleParticles::creator()
{
	return new sampleParticles();
}

/*

	emits particles with color sampled from specified
	shading node/shading engine

*/
MStatus sampleParticles::doIt( const MArgList& args )
{
	unsigned int i;
	bool shadow = 0;
	bool reuse = 0;

	for ( i = 0; i < args.length(); i++ )
		if ( args.asString(i) == MString("-shadow") || 
			args.asString(i) == MString("-s") )
			shadow = 1;
		else if ( args.asString(i) == MString("-reuse") || 
			args.asString(i) == MString("-r") )
			reuse = 1;
		else
			break;
	if ( args.length() - i < 5 )
	{
		displayError( "Usage: sampleParticles [-shadow|-reuse] particleName <shadingEngine|shadingNode.plug> resX resY scale\n"
			"  Example: sampleParticles -shadow particle1 phong1SG 64 64 10;\n"
			"  Example: sampleParticles particle1 file1.outColor 128 128 5;\n" );
		return MS::kFailure;
	}
	if ( reuse && !shadow )	// can only reuse if shadow is turned on
		reuse = 0;

	MString particleName = args.asString( i );
	MString node = args.asString( i+1 );
	int resX = args.asInt( i+2 );
	int resY = args.asInt( i+3 );
	double scale = args.asDouble( i+4 );

	if ( scale <= 0.0 )
		scale = 1.0;

	MFloatArray uCoord, vCoord;
	MFloatPointArray points;
	MFloatVectorArray normals, tanUs, tanVs;

	if ( resX <= 0 )
		resX = 1;
	if ( resY <= 0 )
		resY = 1;

	MString command( "emit -o " );
	command += particleName;
	char tmp[2048];

	float stepU = (float) (1.0 / resX);
	float stepV = (float) (1.0 / resY);

	// stuff sample data by iterating over grid
	// Y is set to arch along the X axis

	int x, y;
	for ( y = 0; y < resY; y++ )
		for ( x = 0; x < resX; x++ )
		{
			uCoord.append( stepU * x );
			vCoord.append( stepV * y );

			float curY = (float) (sin( stepU * (x) * M_PI )*2.0);

			MFloatPoint curPt(
				(float) (stepU * x * scale),
				curY,
				(float) (stepV * y * scale ));

			MFloatPoint uPt(
				(float) (stepU * (x+1) * scale),
				(float) (sin( stepU * (x+1) * M_PI )*2.0),
				(float) (stepV * y * scale ));

			MFloatPoint vPt(
				(float) (stepU * (x) * scale),
				curY,
				(float) (stepV * (y+1) * scale ));

			MFloatVector du, dv, n;
			du = uPt-curPt;
			dv = vPt-curPt;

			n = dv^du;	// normal is based on dU x dV
			n = n.normal();
			normals.append( n );

			du.normal();
			dv.normal();
			tanUs.append( du );
			tanVs.append( dv );

			points.append( curPt );
		}

	// get current camera's world matrix

	MDagPath cameraPath;
	M3dView::active3dView().getCamera( cameraPath );
	MMatrix mat = cameraPath.inclusiveMatrix();
	MFloatMatrix cameraMat( mat.matrix );

	MFloatVectorArray colors, transps;
	if ( MS::kSuccess == MRenderUtil::sampleShadingNetwork( 
			node, 
			points.length(),
			shadow,
			reuse,

			cameraMat,

			&points,
			&uCoord,
			&vCoord,
			&normals,
			&points,
			&tanUs,
			&tanVs,
			NULL,	// don't need filterSize

			colors,
			transps ) )
	{
		fprintf( stderr, "%u points sampled...\n", points.length() );
		for ( i = 0; i < uCoord.length(); i++ )
		{
			sprintf( tmp, " -pos %g %g %g -at velocity -vv %g %g %g -at rgbPP -vv %g %g %g",
				points[i].x,
				points[i].y,
				points[i].z,

				normals[i].x,
				normals[i].y,
				normals[i].z,

				colors[i].x,
				colors[i].y,
				colors[i].z );

			command += MString( tmp );

			// execute emit command once every 512 samples
			if ( i % 512 == 0 )
			{
				fprintf( stderr, "%u...\n", i );
				MGlobal::executeCommand( command, false, false );
				command = MString( "emit -o " );
				command += particleName;
			}
		}

		if ( i % 512 )
			MGlobal::executeCommand( command, true, true );
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

	status = plugin.registerCommand( "sampleParticles", sampleParticles::creator );
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

	status = plugin.deregisterCommand( "sampleParticles" );
	if (!status) {
		status.perror("deregisterCommand");
		return status;
	}

	return status;
}
