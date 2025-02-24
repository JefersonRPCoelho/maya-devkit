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

//
// File: cleanPerFaceAssignmentCmd.cpp
// MEL Command: cleanPerFaceAssignment
// Author: Hiroyuki Haga
// 

#include "cleanPerFaceAssignmentCmd.h"

// Include Maya libary: General
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include <maya/MPlug.h>
#include <maya/MObjectArray.h>
#include <maya/MIntArray.h>
#include <maya/MStringArray.h>

// Include Maya libary: Selection
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>

// Include Maya libary: Nodes
#include <maya/MFnDependencyNode.h>

// Include Maya libary: Polygons
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>

// Include Maya libary: Sets
#include <maya/MFnSet.h>

#ifdef MAYA_PRINT_DEBUG_INFO
#include <maya/MIOStream.h>
#endif

MStatus cleanPerFaceAssignment::doIt( const MArgList& )
//
//	Description:
//		cleanPerfaceAssignment does the followings
//			1. It traces material connections
//			2. If multiple conections to one material is found, it combines them.
//			3. It changes the assignment order
//			4. it returns a string array that can be executed as a MEL command
//
//	Arguments:
//		NONE
//
//	Return Value:
//		It always returns MS:kSuccess as there's no error handling
//
{
	MStatus stat = MS::kSuccess;


	//  Get the selection list and using the iterator to trace them
	MSelectionList list;
	MGlobal::getActiveSelectionList( list );
	MItSelectionList listIt( list);

	for ( listIt.reset(); !listIt.isDone() ; listIt.next() ){

		// Get the DAG path of each element and extend the path to its shape node
		MDagPath path;
		listIt.getDagPath( path );
		path.extendToShape();
	
		// if a DAG Path is MFn::kMesh, it tries to obtain its material

		if ( path.apiType() == MFn::kMesh ){

			// Create a function set for poly geometries, find faces that are assigned as a set
			MFnMesh meshFn( path );
			MObjectArray sets, comps;

			unsigned int instanceNumber = path.instanceNumber();
			meshFn.getConnectedSetsAndMembers( instanceNumber, sets, comps, 1 );

#ifdef MAYA_PRINT_DEBUG_INFO
			// Debug output
			for ( unsigned int i = 0; i < sets.length() ; i++ ){
				MFnSet setFn ( sets[i]);
				MItMeshPolygon faceIt( path, comps[i] );

				cout << "-------------->" << endl;
				cout << setFn.name() << endl;
				cout << "FaceCount:" << faceIt.count() << endl;
				for ( faceIt.reset() ; !faceIt.isDone() ; faceIt.next() ){
					cout << faceIt.index() << " ";
				}
				cout << endl;
				cout << "<--------------" << endl;
				
			}
#endif //MAYA_PRINT_DEBUG_INFO


			// Variable declaration 
			MStringArray SGNames;		// Stores Shading Group name
			MIntArray sameConnectionFlag;	// a flag to check if Shading Group has same names
			MIntArray sameSGFaceCount;	// storoes the number of faces included in each shading group
			MStringArray memberFaceNames;	// stores names of faces included in each shading group

			sameConnectionFlag.clear();
			sameSGFaceCount.clear();

			// ------------------------------------------------
			// 1	Initialization
			// ------------------------------------------------

			// initialization of variables
			for ( unsigned int i = 0; i < sets.length() ; i++ ){

				// SGName Initialization
				//	Append Shading Group name
				MFnSet setFn ( sets[i] );
				SGNames.append( setFn.name() );

				// sameConnectionFlag Initialization
				//	set -1
				sameConnectionFlag.append( -1 );
				
				
				// sameSGFaceCount Initialization
				//	Set the number of faces included in a Shading Group
				MItMeshPolygon tempFaceIt ( path, comps[i] );				
				sameSGFaceCount.append( tempFaceIt.count() );


				// memberFaceNames Initialization
				//	Stores names of faces included in a Shading Group in a way MEL can use
				MString aMemberFaceName;

				MString pathName = path.fullPathName();
				bool optimizeList = true; 
				if (optimizeList)
				{
					int lastIndices[2] = { -1, -1 };
					int currentIndex = -1;
					bool haveAFace = false;
					for ( ;!tempFaceIt.isDone() ; tempFaceIt.next() )
					{
						if (lastIndices[0] == -1)
						{
							lastIndices[0] = tempFaceIt.index();
							lastIndices[1] = tempFaceIt.index();
						}
						else
						{
							currentIndex = tempFaceIt.index();

							// Hit non-contiguous face #. split out a new string
							if (currentIndex > lastIndices[1] + 1)
							{
								aMemberFaceName += (pathName + ".f[" + lastIndices[0] + ":" + lastIndices[1] + "] ");
								lastIndices[0] = currentIndex;
								lastIndices[1] = currentIndex;
							}
							else
								lastIndices[1] = currentIndex;
						}
						haveAFace = true;
					}
					// Only one member. Add it.
					if (haveAFace)
					{
						aMemberFaceName += (pathName + ".f[" + lastIndices[0] + ":" + lastIndices[1] + "] ");
					}
				}
				else
				{
					for ( ;!tempFaceIt.isDone() ; tempFaceIt.next() ){
						aMemberFaceName += (path.fullPathName() + ".f[" + tempFaceIt.index() + "] ");
					}
				}
				memberFaceNames.append( aMemberFaceName );

			}
			
	
			// ------------------------------------------------
			// 2	Finding redundant connections
			// ------------------------------------------------

			// Scan for multiple connections to a shading group and if it exists
			//	combine them
			for ( unsigned int i = 0; i < sets.length() ; i++ ){
				MFnSet setFn ( sets[i]);
				if ( sameConnectionFlag[i] == -1 ){

					for ( unsigned int j = 0; j < sets.length() ; j++ ){
						if ( i != j && sameConnectionFlag[j] == -1 ) {
							MFnSet tempSetFn ( sets[j] );

							if ( setFn.name() == tempSetFn.name() ){
								sameConnectionFlag[j] = i;
								sameSGFaceCount[i] += sameSGFaceCount[j];
								sameSGFaceCount[j] = 0;
								memberFaceNames[i] += memberFaceNames[j];
								memberFaceNames[j] = "";
							}
						}

					}
					sameConnectionFlag[i] = i;
				}
			}

			// delete empties
			unsigned int tail = 0;
			while( tail < sameConnectionFlag.length() ){
				if ( sameSGFaceCount[tail] == 0 ){
					SGNames.remove( tail );
					memberFaceNames.remove( tail );
					sameConnectionFlag.remove( tail );
					sameSGFaceCount.remove( tail );

				} else {
					tail++;
				}
			}


			// ------------------------------------------------
			// 3	Sorting array
			// ------------------------------------------------

			// Sorting preprocess
			//		Simply assign ID from the top
			MIntArray sortMapper;
			for ( unsigned int i = 0; i < sameSGFaceCount.length() ; i++ ){
				sortMapper.append( i );
			}
			bool needToSort = true;
	        
			// Begin sorting
			//		in the order of number of faces 
			while ( needToSort ){
				unsigned int sortCount = 0;
				if ( sameSGFaceCount.length() > 1 ){
					for( unsigned int i = 0; i < ( sameSGFaceCount.length() -1 ) ; i++ ){
						if ( sameSGFaceCount[sortMapper[i]] < sameSGFaceCount[sortMapper[i+1]] ){
							int tmp = sortMapper[i];
							sortMapper[i] = sortMapper[i+1];
							sortMapper[i+1] = tmp;
							sortCount++;
						} else {

						}

					}
				}else {
					break;
				}

				
				// Check if sorting is completed
				if ( sortCount > 0 ){
					needToSort = true; //another sorting required
				} else {
					needToSort = false;//no sorting required and exit loop
				}


			}

			// store the sorted arrary

			//	Variable declaration
			MStringArray sortedSGNames;
			MStringArray sortedMemberFaceNames;
			MIntArray	sortedSGFaceCount;

			//	Store
			for ( unsigned int i = 0; i < sortMapper.length(); i++ ){
				sortedSGNames.append( SGNames[sortMapper[i]] );
				sortedSGFaceCount.append( sameSGFaceCount[sortMapper[i]] );
				sortedMemberFaceNames.append( memberFaceNames[sortMapper[i]]);
			}

			

#ifdef MAYA_PRINT_DEBUG_INFO
			// Debug information output
			for ( unsigned int i = 0; i < sortedSGFaceCount.length() ; i++ ){
				cout << sortedSGNames[i] << "  " << sortedSGFaceCount[i] << endl;
				cout << sortedMemberFaceNames[i] << endl;
			}
			
			for ( unsigned int i = 0; i < sortMapper.length() ; i++ ){
				cout << i << " -- " << sortMapper[i] << endl;
			}
#endif //MAYA_PRINT_DEBUG_INFO
			
			// ------------------------------------------------
			// 4	Output
			// ------------------------------------------------
			//	This outputs a MEL executable string array
			// 	This is to enable step by step excution when it can't be solved to normally 
			
			bool executeImmediate = true;
			for ( unsigned int i = 0; i < sortedSGFaceCount.length() ; i++ ){
				
				MString resultString;
				if ( i == 0 ){

					// Add object base selection MEL command 
					resultString = ("select -r " + path.fullPathName()  +";" );
				} else {
					// Add component base selection MEL command 
					resultString = ("select -r " + sortedMemberFaceNames[i] + ";");
				}

				if (executeImmediate )
					MGlobal::executeCommand( resultString );

				// Add a meterial assignemnet MEL comand 
				if (executeImmediate)
				{
					resultString = ("sets -e -forceElement " + sortedSGNames[i] + ";");
					MGlobal::executeCommand( resultString );
				}
				else
					resultString += ("sets -e -forceElement " + sortedSGNames[i] + ";");

				// Append to the result
				if (!executeImmediate)
					appendToResult( resultString );
			}

		}

	}
	return stat; // TODO: Error handling/Additional ASSERT required. BCurrently it always returns MS::kSuccess
}


void* cleanPerFaceAssignment::creator()
//
//	Description:
//		this method exists to give Maya a way to create new objects
//      of this type. 
//
//	Return Value:
//		a new object of this type
//
{
	return new cleanPerFaceAssignment();
}

cleanPerFaceAssignment::cleanPerFaceAssignment()
//
//	Description:
//		cleanPerFaceAssignment constructor
//
{}

cleanPerFaceAssignment::~cleanPerFaceAssignment()
//
//	Description:
//		cleanPerFaceAssignment destructor
//
{
}


