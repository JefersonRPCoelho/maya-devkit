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
// Produces the command "volumeLight".
//
// Demonstrates the use of the MFnVolumeLight class.
// This example creates a volume light, then queries and sets a number of its attributes.
//
// Example: 
//
//	volumeLight;
//	volumeLight -a $arc -c $coneEndRadius -e $emitAmbient;
//
////////////////////////////////////////////////////////////////////////

#include <math.h>

#include <maya/MIOStream.h>
#include <maya/MSimple.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MPointArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MPoint.h>

#include <maya/MRampAttribute.h>
#include <maya/MFnVolumeLight.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MColorArray.h>
#include <maya/MColor.h>

DeclareSimpleCommand( volumeLight, PLUGIN_COMPANY, "5.0");
#define EQUAL(a,b) ( (((a-b) > -0.00001) && ((a-b) < 0.00001)) ? 1 : 0 )

MStatus volumeLight::doIt( const MArgList& args )
{
	MStatus stat;

	double arc = 180.0f;
	double coneEndRadius = 0.0f;
	MFnVolumeLight::MLightDirection volumeLightDirection = MFnVolumeLight::kOutward;
	MFnVolumeLight::MLightShape lightShape = MFnVolumeLight::kConeVolume;
	bool emitAmbient = true;

	unsigned	i;

	// Parse the arguments.
	for ( i = 0; i < args.length(); i++ )
	{
		if ( MString( "-a" ) == args.asString( i, &stat )
				&& MS::kSuccess == stat)
		{
			double tmp = args.asDouble( ++i, &stat );
			if ( MS::kSuccess == stat )
				arc = tmp;
		}
		else if ( MString( "-c" ) == args.asString( i, &stat )
				&& MS::kSuccess == stat)
		{
			double tmp = args.asDouble( ++i, &stat );
			if ( MS::kSuccess == stat )
				coneEndRadius = tmp;
		}
		else if ( MString( "-e" ) == args.asString( i, &stat )
				&& MS::kSuccess == stat)
		{
			bool tmp = args.asBool( ++i, &stat );
			if ( MS::kSuccess == stat )
				emitAmbient = tmp;
		}

	}

	MFnVolumeLight light;

	light.create( true, &stat);

	cout<<"What's up?";

	if ( MS::kSuccess != stat )
	{
		cout<<"Error creating light."<<endl;
		return stat;
	}
	
	stat = light.setArc ((float)arc);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error setting \"arc\" attribute."<<endl;
		return stat;
	}

	stat = light.setVolumeLightDirection (volumeLightDirection);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error setting \"volumeLightDirection\" attribute."<<endl;
		return stat;
	}

	stat = light.setConeEndRadius ((float)coneEndRadius);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error setting \"coneEndRadius\" attribute."<<endl;
		return stat;
	}

	stat = light.setEmitAmbient (emitAmbient);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error setting \"emitAmbient\" attribute."<<endl;
		return stat;
	}

	stat = light.setLightShape (lightShape);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error setting \"lightShape\" attribute."<<endl;
		return stat;
	}

	double arcGet = light.arc (&stat);
	if ( MS::kSuccess != stat || arcGet != arc)
	{
		cout<<"Error getting \"arc\" attribute."<<endl;
		return stat;
	}

	MFnVolumeLight::MLightDirection volumeLightDirectionGet = light.volumeLightDirection (&stat);
	if ( MS::kSuccess != stat || volumeLightDirectionGet != volumeLightDirection)
	{
		cout<<"Error getting \"volumeLightDirection\" attribute."<<endl;
		return stat;
	}

	double coneEndRadiusGet = light.coneEndRadius (&stat);
	if ( MS::kSuccess != stat || coneEndRadiusGet != coneEndRadius)
	{
		cout<<"Error getting \"coneEndRadius\" attribute."<<endl;
		return stat;
	}

	bool emitAmbientGet = light.emitAmbient (&stat);
	if ( MS::kSuccess != stat || emitAmbientGet != emitAmbient)
	{
		cout<<"Error getting \"emitAmbient\" attribute."<<endl;
		return stat;
	}

	MFnVolumeLight::MLightShape lightShapeGet = light.lightShape (&stat);
	if ( MS::kSuccess != stat || lightShapeGet != lightShape)
	{
		cout<<"Error getting \"lightShape\" attribute."<<endl;
		return stat;
	}

	// Get reference to the penumbra ramp.
	MRampAttribute ramp = light.penumbraRamp (&stat);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error getting \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	MFloatArray a, b;
	MIntArray c,d;

	// Get the entries in the ramp
	ramp.getEntries (d, a, b, c, &stat);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error getting entries from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	// There should be 2 entries by default.
	if (d.length() != 2)
	{
		cout<<"Invalid number of entries in \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	MFloatArray a1, b1;
	MIntArray c1;

	// Prepare an array of entries to add.
	// In this case we are just adding 1 more entry
	// at position 0.5 with a curve value of 0.25 and a linear interpolation.
	a1.append (0.5f);
	b1.append (0.25f);
	c1.append (MRampAttribute::kLinear);

	// Add it to the curve ramp
	ramp.addEntries (a1, b1, c1, &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error adding entries to \"penumbraRamp\" attribute."<<endl;
		return stat;
	}


	// Get the entries to make sure that the above add actually worked.
	MFloatArray a2, b2;
	MIntArray c2,d2;
	ramp.getEntries (d2, a2, b2, c2, &stat);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error getting entries from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}
	if ( a.length() + a1.length() != a2.length())
	{
		cout<<"Invalid number of entries in \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	// Now try to interpolate the value at a point
	float newVal = -1;
	ramp.getValueAtPosition(.3f, newVal, &stat);

	if ( MS::kSuccess != stat )
	{
		cout<<"Error interpolating value from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}
	if ( !EQUAL(newVal, .15f))
	{
		cout<<"Invalid interpolation in  \"penumbraRamp\" expected .15 got "<<newVal
			<<" ."<<endl;
	}

	// Try to delete an entry in an incorrect manner. 
	// This delete will work because there is an entry at 0, 
	// However we should never do it this way, because the entries
	// array can become sparse, so trying to delete an entry without 
	// checking whether an entry exists at that index can cause a failure.
	MIntArray entriesToDelete;
	entriesToDelete.append (0);
	ramp.deleteEntries (entriesToDelete, &stat);
	if ( MS::kSuccess != stat )
	{
		cout<<"Error deleting entries from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	// Check to see whether the above delete worked.
	// As mentioned earlier it did work, but we shouldn't do it this way.
	// To illustrate why we shouldn't do it this way, we'll try to delete
	// entry at index 0 ( this no longer exists)
	ramp.getEntries (d2, a2, b2, c2, &stat);

	if ( a2.length() != 2)
	{
		cout<<"Invalid number of entries in \"penumbraRamp\" attribute."<<endl;
		return stat;
	}
	// Trying to delete entry at 0.
	entriesToDelete.clear();
	entriesToDelete.append (0);
	ramp.deleteEntries (entriesToDelete, &stat);

	// It will fail because no entry exists.
	if ( MS::kSuccess == stat)
	{
		cout<<"Error deleting entries from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}
	if ( a2.length() != 2)
	{
		cout<<"Invalid number of entries in \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	// The proper way to delete is to retrieve the index by calling "getEntries"
	ramp.getEntries (d2, a2, b2, c2, &stat);
	entriesToDelete.clear();
	entriesToDelete.append (d2[0]);

	// Delete the first logical entry in the entry array.
	ramp.deleteEntries (entriesToDelete, &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error deleting entries from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	// There should be only 1 entry left.
	ramp.getEntries (d2, a2, b2, c2, &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error getting entries from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}
	entriesToDelete.clear();
	entriesToDelete.append (d2[0]);

	// Can't delete the last entry, should return failure.
	ramp.deleteEntries (entriesToDelete, &stat);
	if ( MS::kSuccess == stat)
	{
		cout<<"Error deleting entries from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}

	ramp.setPositionAtIndex (0.0f, d2[0], &stat);
	if ( MS::kSuccess != stat)
	{
		printf("Error setting position at index: %d, of \"penumbraRamp\" attribute.\n", d2[0]);
		return stat;
	}

	ramp.setValueAtIndex (1.0f, d2[0], &stat);
	if ( MS::kSuccess != stat)
	{
		printf("Error setting value at index: %d, of \"penumbraRamp\" attribute.\n", d2[0]);
		return stat;
	}

	ramp.setInterpolationAtIndex (MRampAttribute::kNone, d2[0], &stat);
	if ( MS::kSuccess != stat)
	{
		printf("Error setting interpolation at index: %d, of \"penumbraRamp\" attribute.\n", d2[0]);
		return stat;
	}

  	MRampAttribute ramp2 = light.colorRamp (&stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error getting  \"colorRamp\" attribute."<<endl;
		return stat;
	}	
	MFloatArray a3;
	MColorArray b3;
	MIntArray c3,d3;

  	// Get the entries in the ramp
	ramp2.getEntries (d3, a3, b3, c3, &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error getting entries from \"colorRamp\" attribute."<<endl;
		return stat;
	}
	// There should be 2 entries by default.
	if ( d3.length() != 2)
	{
		cout<<"Invalid number of entries in \"colorRamp\" attribute."<<endl;
		return stat;
	}

	MFloatArray a4;
	MColorArray b4;
	MIntArray c4;

	// Prepare an array of entries to add.
	// In this case we are just adding 1 more entry
	// at position 0.5 withe curve value of 0.5 and a linear interpolation.
	a4.append (0.5f);
	b4.append (MColor (0.0f, 0.0f, 0.75f));
	c4.append (MRampAttribute::kLinear);

	// Add it to the curve ramp
	ramp2.addEntries (a4, b4, c4, &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error adding entries to \"colorRamp\" attribute."<<endl;
		return stat;
	}
	// Get the entries to make sure that the above add actually worked.
	MFloatArray a5;
	MColorArray b5;
	MIntArray c5,d5;
	ramp2.getEntries (d5, a5, b5, c5, &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error getting entries from \"colorRamp\" attribute."<<endl;
		return stat;
	}
	if (  a3.length() + a4.length() != a5.length())
	{
		cout<<"Invalid number of entries in \"colorRamp\" attribute."<<endl;
		return stat;
	}

	// Now try to interpolate the color at a point
	MColor newCol(0.0, 0.0, 0.0);
	ramp2.getColorAtPosition(.3f, newCol, &stat);

	if ( MS::kSuccess != stat )
	{
		cout<<"Error interpolating color from \"penumbraRamp\" attribute."<<endl;
		return stat;
	}
	if ( !EQUAL(newCol[2], .45))
	{
		cout<<"Invalid color interpolation in  \"colorRamp\" expected .45 got "<<newCol[2]<<endl;
	}

	MColor clr (0.5, 0.5, 0.0);
	ramp2.setColorAtIndex (clr, d5[0], &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error setting color at index: "<<d5[0]
			<<", of \"colorRamp\" attribute."<<endl;
		return stat;
	}

	ramp2.setInterpolationAtIndex (MRampAttribute::kSpline, d5[1], &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error setting interpolation at index: "<<d5[1]
			<<", of \"colorRamp\" attribute."<<endl;
		return stat;
	}

	// The following demonstrates some of the new methods of the MRampAttribute API
	MIntArray indices;
	MFloatArray pos;
	MColorArray colors;
	MIntArray interps;
	ramp2.getEntries( indices, pos, colors, interps, &stat);
	if ( MS::kSuccess != stat)
	{
		cout<<"Error getting entries from \"colorRamp\" attribute."<<endl;
		return stat;
	}

	for (unsigned int i=0; i<colors.length(); i++)
	{
		colors[i] = colors[i]*2;
		interps[i] = MRampAttribute::kSpline;
	}

	// set color ramp with new values
	stat = ramp2.setRamp( colors, pos, interps );
	if ( MS::kSuccess != stat)
	{
		cout<<"Error setting values on \"colorRamp\" attribute."<<endl;
		return stat;
	}

	// sort entries by position, this will regenerate the indices back from 0
	stat = ramp2.sort( true );
	if ( MS::kSuccess != stat)
	{
		cout<<"Error sorting entries from \"colorRamp\" attribute."<<endl;
		return stat;
	}

	// Check for valid indices before setting interpolation types
	if ( ramp2.hasIndex( 0 ) )
	{
		ramp2.setInterpolationAtIndex (MRampAttribute::kSpline, 0 );
	}

	if ( ramp2.hasIndex( 56 ) )
	{
		ramp2.setInterpolationAtIndex( MRampAttribute::kSpline, 56 );
	}

	return stat;
}
