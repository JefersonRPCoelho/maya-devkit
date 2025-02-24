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
// particleSystemInfoCmd
//
// Demonstrates the use of the new MFnParticleSystem class for retrieving
// particle information.  The number of particle positions followed by
// the positions are printed to the script editor.
//
//  Syntax:
//  	particleSystemInfo $particleNodeName;
//
//  Example:
// 	particleSystemInfo particleShape2;
//
////////////////////////////////////////////////////////////////////////

#include <assert.h>

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MPointArray.h>
#include <maya/MVectorArray.h>
#include <maya/MFloatMatrix.h>
#include <maya/MArgList.h>
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPxCommand.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MFnPlugin.h>
#include <maya/MObjectArray.h>

#define mCommandName "particleSystemInfo"	// Command name

#define CHECKRESULT(stat,msg)     \
        if ( MS::kSuccess != stat ) { \
                cerr << msg << endl;      \
        }

class particleSystemInfoCmd : public MPxCommand
{
public:
				 particleSystemInfoCmd();
    		~particleSystemInfoCmd() override;

    MStatus doIt ( const MArgList& args ) override;

    static void* creator();

private:

	static MStatus nodeFromName(MString name, MObject & obj);

	MStatus parseArgs ( const MArgList& args );

	MObject particleNode;
};

particleSystemInfoCmd::particleSystemInfoCmd()
{
}

particleSystemInfoCmd::~particleSystemInfoCmd() 
{
}


//
//
///////////////////////////////////////////////////////////////////////////////

MStatus particleSystemInfoCmd::nodeFromName(MString name, MObject & obj)
{
	MSelectionList tempList;
	tempList.add( name );
	if ( tempList.length() > 0 ) 
	{
		tempList.getDependNode( 0, obj );
		return MS::kSuccess;
	}
	return MS::kFailure;
}




MStatus particleSystemInfoCmd::parseArgs( const MArgList& args )
{
	// Parse the arguments.
	MStatus stat = MS::kSuccess;

	if( args.length() > 1 )
	{
		MGlobal::displayError( "Too many arguments." );
		return MS::kFailure;
	}

	if( args.length() == 1 )
	{
	        MString particleName = args.asString( 0, &stat );
		CHECKRESULT(stat, "Failed to parse particle node name argument." );

		nodeFromName( particleName, particleNode );
		
		if( !particleNode.isNull() && !particleNode.hasFn( MFn::kParticle ) )
		{
		        MGlobal::displayError( "The named node is not a particle system." );
			return MS::kFailure;
		}
	} 
	return MS::kSuccess;
}


//
// Main routine
///////////////////////////////////////////////////////////////////////////////
MStatus particleSystemInfoCmd::doIt( const MArgList& args )
{
	MStatus stat = parseArgs( args );
	if( stat != MS::kSuccess ) return stat;

	if( particleNode.isNull() ) {
	        MObject parent;
		MFnParticleSystem dummy;
		particleNode = dummy.create(&stat);
		CHECKRESULT(stat,"MFnParticleSystem::create(status) failed!");

		MFnParticleSystem ps( particleNode, &stat );
		CHECKRESULT(stat,"MFnParticleSystem::MFnParticleSystem(MObject,status) failed!");

		MPointArray posArray;
		posArray.append(MPoint(-5,  5, 0));
		posArray.append(MPoint(-5, 10, 0));

		MVectorArray velArray;
		velArray.append(MPoint(1, 1, 0));
		velArray.append(MPoint(1, 1, 0));
		stat = ps.emit(posArray, velArray);
		CHECKRESULT(stat,"MFnParticleSystem::emit(posArray,velArray) failed!");

		stat = ps.emit(MPoint(5,  5, 0));
		CHECKRESULT(stat,"MFnParticleSystem::emit(pos) failed!");
		stat = ps.emit(MPoint(5, 10, 0));
		CHECKRESULT(stat,"MFnParticleSystem::emit(pos) failed!");

		stat = ps.saveInitialState();
		CHECKRESULT(stat,"MFnParticleSystem::saveInitialState() failed!");

		MVectorArray accArray;
		accArray.setLength(4);
		for( unsigned int i=0; i<accArray.length(); i++ )
		{
		        MVector& acc = accArray[i];
			acc.x = acc.y = acc.z = 3.0;
		}
		MString accName("acceleration");
		ps.setPerParticleAttribute( accName, accArray, &stat );
		CHECKRESULT(stat,"MFnParticleSystem::setPerParticleAttribute(vectorArray) failed!");
	} 

	MFnParticleSystem ps( particleNode, &stat );
	CHECKRESULT(stat,"MFnParticleSystem::MFnParticleSystem(MObject,status) failed!");

	if( ! ps.isValid() )
	{
		MGlobal::displayError( "The function set is invalid!" );
		return MS::kFailure;
	}

	const MString name = ps.particleName();
	const MFnParticleSystem::RenderType psType = ps.renderType();
	const unsigned int count = ps.count();

	const char* typeString = NULL;
	switch( psType )
	{
	case MFnParticleSystem::kCloud:
		typeString = "Cloud";
		break;
	case MFnParticleSystem::kTube:
		typeString = "Tube system";
		break;
	case MFnParticleSystem::kBlobby:
		typeString = "Blobby";
		break;
	case MFnParticleSystem::kMultiPoint:
		typeString = "MultiPoint";
		break;
	case MFnParticleSystem::kMultiStreak:
		typeString = "MultiStreak";
		break;
	case MFnParticleSystem::kNumeric:
		typeString = "Numeric";
		break;
	case MFnParticleSystem::kPoints:
		typeString = "Points";
		break;
	case MFnParticleSystem::kSpheres:
		typeString = "Spheres";
		break;
	case MFnParticleSystem::kSprites:
		typeString = "Sprites";
		break;
	case MFnParticleSystem::kStreak:
		typeString = "Streak";
		break;
	default:
		typeString = "Particle system";
		assert( false );
		break;
	}

	char buffer[256];

	sprintf( buffer, "%s \"%s\" has %u primitives.", typeString, name.asChar(), count );
	MGlobal::displayInfo( buffer );

	unsigned i;

	MIntArray ids;
	ps.particleIds( ids );

	sprintf( buffer, "count : %u ", count );
	MGlobal::displayInfo( buffer );
	sprintf( buffer, "%u ids.", ids.length() );
	MGlobal::displayInfo( buffer );

	assert( ids.length() == count );
	for( i=0; i<ids.length(); i++ )
	{
		sprintf( buffer, "id %d  ", ids[i] );
		MGlobal::displayInfo( buffer );
	}

	MVectorArray positions;
	ps.position( positions );
	assert( positions.length() == count );
	for( i=0; i<positions.length(); i++ )
	{
		MVector& p = positions[i];
		sprintf( buffer, "pos %f %f %f  ", p[0], p[1], p[2] );
		MGlobal::displayInfo( buffer );
	}

	MVectorArray vels;
	ps.velocity( vels );
	assert( vels.length() == count );
	for( i=0; i<vels.length(); i++ )
	{
		const MVector& v = vels[i];
		sprintf( buffer, "vel %f %f %f  ", v[0], v[1], v[2] );
		MGlobal::displayInfo( buffer );
	}

	MVectorArray accs;
	ps.acceleration( accs );
	assert( accs.length() == count );
	for( i=0; i<accs.length(); i++ )
	{
		const MVector& a = accs[i];
		sprintf( buffer, "acc %f %f %f  ", a[0], a[1], a[2] );
		MGlobal::displayInfo( buffer );
	}

	bool flag = ps.isDeformedParticleShape(&stat);
	CHECKRESULT(stat,"MFnParticleSystem::isDeformedParticleShape() failed!");
	if( flag ) {
	        MObject obj = ps.originalParticleShape(&stat);
		CHECKRESULT(stat,"MFnParticleSystem::originalParticleShape() failed!");
		if( obj != MObject::kNullObj ) {
		        MFnParticleSystem ps( obj );
			sprintf( buffer, "original particle shape : %s ", ps.particleName().asChar() );
			MGlobal::displayInfo( buffer );
		}
	}

	flag = ps.isDeformedParticleShape(&stat);
	CHECKRESULT(stat,"MFnParticleSystem::isDeformedParticleShape() failed!");
	if( !flag ) {
	        MObject obj = ps.deformedParticleShape(&stat);
		CHECKRESULT(stat,"MFnParticleSystem::deformedParticleShape() failed!");
		if( obj != MObject::kNullObj ) {
		        MFnParticleSystem ps( obj );
			sprintf( buffer, "deformed particle shape : %s ", ps.particleName().asChar() );
			MGlobal::displayInfo( buffer );
		}
	}

	if( ids.length() == positions.length() &&
	    ids.length() == vels.length()      &&
	    ids.length() == accs.length()       ) { 
	        setResult( int(ids.length()) );
	} else {
	        setResult( int(-1) );
	}

	return MS::kSuccess;
}


//
//
///////////////////////////////////////////////////////////////////////////////

void * particleSystemInfoCmd::creator() { return new particleSystemInfoCmd(); }


MStatus initializePlugin( MObject obj )
{
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.5", "Any");

    MStatus status = plugin.registerCommand(mCommandName,
											particleSystemInfoCmd::creator );
	if (!status) 
		status.perror("registerCommand");

    return status;
}


MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin( obj );

    MStatus status = plugin.deregisterCommand(mCommandName);
	if (!status) 
		status.perror("deregisterCommand");

    return status;
}
