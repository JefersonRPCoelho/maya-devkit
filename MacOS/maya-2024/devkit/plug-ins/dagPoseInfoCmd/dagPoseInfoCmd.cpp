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
// Description:
//   This is an example of a command that writes information about the
//   dag pose of selected joints out to a file.
//
//   To use the command, select the joints that you want data for and
//   then type the command, being sure to use the -f/-file flag to specify
//   the name of the file to write to.
//
//      dagPoseInfo -f <fileName>
//
//   For example:
//
//      dagPoseInfo -f "C:/temp/poseData"
//
//   The output format used is:
//      <jointName>
//      <poseName>
//      worldMatrix
//      1 0 0 0
//      0 1 0 0
//      0 0 1 0
//      0 0 0 1
//      matrix
//      1 0 0 0
//      0 1 0 0
//      0 0 1 0
//      0 0 0 1
// 
//   Note that the pose node stores the local matrix data in a transformation
//   matrix, so that if one wanted to extract only the rotation components
//   of the pose rather than the entire local matrix, one could do so using
//   the MTransformationMatrix function set.
//
//   Also note that if you want just the bindPose data, rather than data about
//   all of the poses on a joint, you could restrict the output to dagPose
//   nodes for which the "dagPose" attribute is true.
//

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
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnAttribute.h>

#include <maya/MIOStream.h>

#define CheckError(stat,msg)		\
	if ( MS::kSuccess != stat ) {	\
		displayError(msg);			\
		continue;					\
	}


class dagPoseInfo : public MPxCommand
{
public:
                dagPoseInfo();
        ~dagPoseInfo() override;

	MStatus		parseArgs( const MArgList& args );
    MStatus     doIt ( const MArgList& args ) override;
    MStatus     redoIt () override;
    MStatus     undoIt () override;
    bool        isUndoable() const override;

    static      void* creator();

private:
	void 		printDagPoseInfo(MObject& dagPoseNode, unsigned index);
	bool 		findDagPose(MObject& jointNode);
	FILE*		file;
};

dagPoseInfo::dagPoseInfo():
file(NULL)
{
}

dagPoseInfo::~dagPoseInfo() {}

void* dagPoseInfo::creator()
{
    return new dagPoseInfo;
}

bool dagPoseInfo::isUndoable() const
{
    return false;
}

MStatus dagPoseInfo::undoIt()
{
    return MS::kSuccess;
}

MStatus dagPoseInfo::parseArgs( const MArgList& args )
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

void dagPoseInfo::printDagPoseInfo(MObject& dagPoseNode, unsigned index)
//
// Description:
//   Given a dagPose and an index corresponding to a joint, print out
//   the matrix info for the joint.
// Return:
//	 None.
//
{
	MFnDependencyNode nDagPose(dagPoseNode);
	fprintf(file,"%s\n",nDagPose.name().asChar());

	// construct plugs for this joints world and local matrices
	//
	MObject aWorldMatrix = nDagPose.attribute("worldMatrix");
	MPlug pWorldMatrix(dagPoseNode,aWorldMatrix);
	pWorldMatrix.selectAncestorLogicalIndex(index,aWorldMatrix);
	
	MObject aMatrix = nDagPose.attribute("xformMatrix");
	MPlug pMatrix(dagPoseNode,aMatrix);
	pMatrix.selectAncestorLogicalIndex(index,aMatrix);

	// get and print the world matrix data
	//
	MObject worldMatrix, xformMatrix;
	MStatus status = pWorldMatrix.getValue(worldMatrix);
	if (MS::kSuccess != status) {
		displayError("Problem retrieving world matrix.");
	} else {
		bool foundMatrix = 0;
		MFnMatrixData dMatrix(worldMatrix);
		MMatrix wMatrix = dMatrix.matrix(&status);
		if (MS::kSuccess == status) {
			foundMatrix = 1;
			unsigned jj,kk;
			fprintf(file,"worldMatrix\n");
			for (jj = 0; jj < 4; ++jj) {
				for (kk = 0; kk < 4; ++kk) {
					double val = wMatrix(jj,kk);
					fprintf(file,"%f ",val);
				}
				fprintf(file,"\n");
			}
		}
		if (!foundMatrix) {
			displayError("Error getting world matrix data.");
		}
	}

	// get and print the local matrix data
	//
	status = pMatrix.getValue(xformMatrix);
	if (MS::kSuccess != status) {
		displayError("Problem retrieving xform matrix.");
	} else {
		bool foundMatrix = 0;
		MFnMatrixData dMatrix(xformMatrix);
		if (dMatrix.isTransformation()) {
			MTransformationMatrix xform = dMatrix.transformation(&status);
			if (MS::kSuccess == status) {
				foundMatrix = 1;
				MMatrix xformAsMatrix = xform.asMatrix();
				unsigned jj,kk;
				fprintf(file,"matrix\n");
				for (jj = 0; jj < 4; ++jj) {
					for (kk = 0; kk < 4; ++kk) {
						double val = xformAsMatrix(jj,kk);
						fprintf(file,"%f ",val);
					}
					fprintf(file,"\n");
				}
			}
		}
		if (!foundMatrix) {
			displayError("Error getting local matrix data.");
		}
	}
}

bool dagPoseInfo::findDagPose(MObject& jointNode)
//
// Description:
//   Given a joint, check for connected dag pose nodes.
//   For each pose found, write out the pose info.
// Return:
//	 If one or more poses is found, return true, else return false.
//
{
	bool rtn = 0; // return 1 if we find a pose

	MStatus status;
	MFnDependencyNode fnJoint(jointNode);
	MObject aBindPose = fnJoint.attribute("bindPose",&status);

	if (MS::kSuccess == status) {
		unsigned connLength = 0;
		MPlugArray connPlugs;
		MPlug pBindPose(jointNode,aBindPose);
		pBindPose.connectedTo(connPlugs,false,true);
		connLength = connPlugs.length();
		for (unsigned ii = 0; ii < connLength; ++ii) {
			if (connPlugs[ii].node().apiType() == MFn::kDagPose) {
				MObject aMember = connPlugs[ii].attribute();
				MFnAttribute fnAttr(aMember);
				if (fnAttr.name() == "worldMatrix") {
					unsigned jointIndex = connPlugs[ii].logicalIndex();

					fprintf(file,"%s\n",fnJoint.name().asChar());
					MObject jointObject = connPlugs[ii].node();
					printDagPoseInfo(jointObject,jointIndex);
					rtn = 1;
				}
			}
		}
	}
	return rtn;
}

MStatus dagPoseInfo::doIt( const MArgList& args )
{
	// parse args to get the file name from the command-line
	//
	MStatus stat = parseArgs(args);
	if (stat != MS::kSuccess) {
		return stat;
	}
	
	unsigned int count = 0;

	// Get the selected joints/transforms, and for each of them print
	// out the dagPose info
	//
	MSelectionList slist;
	MGlobal::getActiveSelectionList( slist );
	MItSelectionList itr( slist );

	for (; !itr.isDone(); itr.next() )
	{
		MObject depNode;
		itr.getDependNode(depNode);
		if (depNode.apiType() == MFn::kJoint) {
			if (findDagPose(depNode)) {
				count++;
			}
		}
	}

	fclose(file);
	if (0 == count) {
		displayError("No poses were found on the selected joints.");
		return MS::kFailure;
	}
	
	return MS::kSuccess;
}

MStatus dagPoseInfo::redoIt()
{
    clearResult();
	setResult( (int) 1);
    return MS::kSuccess;
}

MStatus initializePlugin( MObject obj )
{
	MStatus   status;
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

    status = plugin.registerCommand( "dagPoseInfo", dagPoseInfo::creator );
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

    status = plugin.deregisterCommand( "dagPoseInfo" );
	if (!status) {
		status.perror("deregisterCommand");
	}
    return status;
}
