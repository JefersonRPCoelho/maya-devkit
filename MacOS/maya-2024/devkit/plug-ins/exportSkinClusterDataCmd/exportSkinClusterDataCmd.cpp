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
// Produces the MEL command "exportSkinClusterData".
// 
// This example demonstrates how to find all the skin cluster nodes and uses the MFnSkinCluster function
// set and MItGeometry iterator to export weights per CV for all the geometry that are bound as a skin to a skeleton.
//
// To use this plug-in, build a skeleton and bind geometry using the "Smooth Bind" feature and execute the command: 
//
//	exportSkinClusterData -f <fileName>;
//
// For example:
//
//	exportSkinClusterData -f "C:/temp/skinData"
//
//	The output format used is:
// 
//	skin_path_name vertex_count influence_count
//	influence_1 influence_2 influence_3 .... influence_n
//	vertex_index weight_1 weight_2 weight_3 ... weight_n
//
// Notes:
//   For each vertex index, a weight value is written out corresponding
//   to each of the n influence objects, even if the weight is 0.
//
//   If all of the vertices in the skin are bound as skin, the vertex_count
//   will equal the total number of vertices in the skin object, and
//   the vertex_index values will be sequential. However, if only some of the vertices were bound as skin, the vertex_count
//   will only reflect the count of skin vertices and the vertex_index values will be non-sequential.
//
////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>
#include <maya/MFnPlugin.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MObjectArray.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItGeometry.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnSingleIndexedComponent.h>

#include <maya/MIOStream.h>

#define CheckError(stat,msg)		\
	if ( MS::kSuccess != stat ) {	\
		displayError(msg);			\
		continue;					\
	}


class exportSkinClusterData : public MPxCommand
{
public:
                exportSkinClusterData();
        ~exportSkinClusterData() override;

	MStatus		parseArgs( const MArgList& args );
    MStatus     doIt ( const MArgList& args ) override;
    MStatus     redoIt () override;
    MStatus     undoIt () override;
    bool        isUndoable() const override;

    static      void* creator();

private:
	FILE*		file;
};

exportSkinClusterData::exportSkinClusterData():
file(NULL)
{
}

exportSkinClusterData::~exportSkinClusterData() {}

void* exportSkinClusterData::creator()
{
    return new exportSkinClusterData;
}

bool exportSkinClusterData::isUndoable() const
{
    return false;
}

MStatus exportSkinClusterData::undoIt()
{
    return MS::kSuccess;
}

MStatus exportSkinClusterData::parseArgs( const MArgList& args )
//
// There is one mandatory flag: -f/-file <filename>
//
{
	MStatus     	stat;
	MString     	arg;
	MString			fileName;
	const MString	fileFlag			("-f");
	const MString	fileFlagLong		("-file");

	// Parse the arguments.
	for ( unsigned int i = 0; i < args.length(); i++ ) {
		arg = args.asString( i, &stat );
		if (!stat)              
			continue;
				
		if ( arg == fileFlag || arg == fileFlagLong ) {
			// get the file name
			//
			if (i == args.length()-1) {
				arg += ": must specify a file name";
				displayError(arg);
				return MS::kFailure;
			}
			i++;
			args.get(i, fileName);
		}
		else {
			arg += ": unknown argument";
			displayError(arg);
			return MS::kFailure;
		}
	}

	file = fopen(fileName.asChar(),"wb");
	if (!file) {
		MString openError("Could not open: ");
		openError += fileName;
		displayError(openError);
		stat = MS::kFailure;
	}
	
	return stat;
}


MStatus exportSkinClusterData::doIt( const MArgList& args )
{
	// parse args to get the file name from the command-line
	//
	MStatus stat = parseArgs(args);
	if (stat != MS::kSuccess) {
		return stat;
	}
	
	unsigned int count = 0;
	
	// Iterate through graph and search for skinCluster nodes
	//
	MItDependencyNodes iter( MFn::kInvalid);
	for ( ; !iter.isDone(); iter.next() ) {
		MObject object = iter.thisNode();
		if (object.apiType() == MFn::kSkinClusterFilter) {
			count++;
			
			// For each skinCluster node, get the list of influence objects
			//
			MFnSkinCluster skinCluster(object);
			MDagPathArray infs;
			MStatus stat;
			unsigned int nInfs = skinCluster.influenceObjects(infs, &stat);
			CheckError(stat,"Error getting influence objects.");

			if (0 == nInfs) {
				stat = MS::kFailure;
				CheckError(stat,"Error: No influence objects found.");
			}
			
			// loop through the geometries affected by this cluster
			//
			unsigned int nGeoms = skinCluster.numOutputConnections();
			for (unsigned int ii = 0; ii < nGeoms; ++ii) {
				unsigned int index = skinCluster.indexForOutputConnection(ii,&stat);
				CheckError(stat,"Error getting geometry index.");

				// get the dag path of the ii'th geometry
				//
				MDagPath skinPath;
				stat = skinCluster.getPathAtIndex(index,skinPath);
				CheckError(stat,"Error getting geometry path.");

				// iterate through the components of this geometry
				//
				MItGeometry gIter(skinPath);

				// print out the path name of the skin, vertexCount & influenceCount
				//
				fprintf(file,
						"%s %d %u\n",skinPath.partialPathName().asChar(),
						gIter.count(),
						nInfs);
				
				// print out the influence objects
				//
				for (unsigned int kk = 0; kk < nInfs; ++kk) {
					fprintf(file,"%s ",infs[kk].partialPathName().asChar());
				}
				fprintf(file,"\n");
			
				for ( /* nothing */ ; !gIter.isDone(); gIter.next() ) {
					MObject comp = gIter.currentItem(&stat);
					CheckError(stat,"Error getting component.");

					// Get the weights for this vertex (one per influence object)
					//
					MDoubleArray wts;
					unsigned int infCount;
					stat = skinCluster.getWeights(skinPath,comp,wts,infCount);
					CheckError(stat,"Error getting weights.");
					if (0 == infCount) {
						stat = MS::kFailure;
						CheckError(stat,"Error: 0 influence objects.");
					}

					// Output the weight data for this vertex
					//
					fprintf(file,"%d ",gIter.index());
					for (unsigned int jj = 0; jj < infCount ; ++jj ) {
						fprintf(file,"%f ",wts[jj]);
					}
					fprintf(file,"\n");
				}
			}
		}
	}

	if (0 == count) {
		displayError("No skinClusters found in this scene.");
	}
	fclose(file);
	return MS::kSuccess;
}

MStatus exportSkinClusterData::redoIt()
{
    clearResult();
	setResult( (int) 1);
    return MS::kSuccess;
}

MStatus initializePlugin( MObject obj )
{
	MStatus   status;
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

    status = plugin.registerCommand( "exportSkinClusterData", exportSkinClusterData::creator );
	if (!status) {
		status.perror("registerCommand");
		return status;
	}

    return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus   status;
    MFnPlugin plugin( obj );

    status = plugin.deregisterCommand( "exportSkinClusterData" );
	if (!status) {
		status.perror("deregisterCommand");
	}
    return status;
}
