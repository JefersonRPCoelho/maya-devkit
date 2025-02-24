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
//	File Name:	animExportUtil.h
//
//	Description: an animation export utility which illustrates how to
//	use the MAnimUtil animation helper class, as well as how to export
//	animation using the Maya .anim format
//
//

// *****************************************************************************

// INCLUDED HEADER FILES

#include <stdlib.h>
#include <string.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MFnPlugin.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MDagPath.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectArray.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnSet.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>

#include <maya/MFnAnimCurve.h>
#include <maya/MAnimUtil.h>

#include "animExportUtil.h"
#include "animFileExport.h"

#include <fstream>

// *****************************************************************************

// HELPER METHODS

// *****************************************************************************

#ifndef min
static inline double
min (double a, double b)
{
	return (a < b ? a : b);
}
#endif

#ifndef max
static inline double
max (double a, double b)
{
	return (a > b ? a : b);
}
#endif

// *****************************************************************************

// PUBLIC MEMBER FUNCTIONS

TanimExportUtil::TanimExportUtil()
{
}

TanimExportUtil::~TanimExportUtil()
{
}

MStatus
TanimExportUtil::writer (
	const MFileObject &file,
	const MString &/*options*/,
	FileAccessMode mode
)
{
	// Create the export file
	//
    std::ofstream animFile (file.expandedFullName().asChar());

	// Create a selection list to hold the objects we want to export
	//
	MSelectionList list;
	if (mode == kExportActiveAccessMode) {
		// Use the active list
		//
		MGlobal::getActiveSelectionList (list);
	}
	else {
		// Create a selection list with all the objects in the world
		//
		// Add the top level dag objects
		//
		MItDag dagIt (MItDag::kBreadthFirst);
		for (dagIt.next(); !dagIt.isDone(); dagIt.next()) {
			MDagPath path;
			if (dagIt.getPath (path) != MS::kSuccess) {
				continue;
			}
			list.add (path);
			// We only want the top level objects (since we will walk
			// down the hierarchy later
			//
			dagIt.prune ();
		}
		// Gather the rest of the dependency nodes
		//
		MItDependencyNodes nodeIt;
		for (; !nodeIt.isDone(); nodeIt.next()) {
			MObject node = nodeIt.thisNode();
			if (node.isNull()) {
				continue;
			}
			// We have already saved the dag objects, so skip them here
			//
			if (node.hasFn (MFn::kDagNode)) {
				continue;
			}
			// Watch out for characters, and only write the top level
			// character
			//
			if (node.hasFn (MFn::kCharacter)) {
				// Find out which (if any) sets this node belongs to
				//
				// Get the message attribute
				//
				MFnDependencyNode fnNode (node);
				MObject aMessage = fnNode.attribute (MString ("message"));
				MPlug messagePlug (node, aMessage);
				// Now find what it is connected to as a source plug
				//
				MPlugArray srcPlugArray;
				messagePlug.connectedTo (srcPlugArray, false, true);
				// Now walk through each connection and see if any is to
				// another character
				//
				unsigned int numPlugs = srcPlugArray.length();
				bool belongsToCharacter = false;
				for (unsigned int i = 0; (i < numPlugs) && !belongsToCharacter; i++) {
					const MPlug &plug = srcPlugArray[i];
					if (!plug.node().hasFn (MFn::kCharacter)) {
						continue;
					}
					belongsToCharacter = true;
				}
				if (!belongsToCharacter) {
					list.add (node);
				}
				continue;
			}
			// A superfluous test to filter out unanimated objects, just to
			// show how to use MAnimUtil
			//
			if (!MAnimUtil::isAnimated (node)) {
				continue;
			}
			list.add (node);
		}
	}

	// The Maya .anim format needs to know the bounds of the animation
	// since it could be used in pasteKey operations (which support
	// scaling in time), so gather that information now
	//
	// Find all the plugs that are animated in our selection list
	//
	MPlugArray animatedPlugs;
	MAnimUtil::findAnimatedPlugs (list, animatedPlugs);
	unsigned int numPlugs = animatedPlugs.length();
	bool hasTime = false;
	double startTime = 0.0;
	double endTime = 0.0;
	bool hasUnitless = false;
	double startUnitless = 0.0;
	double endUnitless = 0.0;
	unsigned int i;
	// For each animated plug, determine the bounds of the animation
	//
	for (i = 0; i < numPlugs; i++) {
		MPlug plug = animatedPlugs[i];
		MObjectArray animation;
		// Find the animation curve(s) that animate this plug
		//
		if (!MAnimUtil::findAnimation (plug, animation)) {
			continue;
		}
		unsigned int numCurves = animation.length();
		for (unsigned int j = 0; j < numCurves; j++) {
			MObject animCurveNode = animation[j];
			if (!animCurveNode.hasFn (MFn::kAnimCurve)) {
				continue;
			}
			MFnAnimCurve animCurve (animCurveNode);
			unsigned int numKeys = animCurve.numKeys();
			if (numKeys == 0) {
				continue;
			}
			if (animCurve.isUnitlessInput()) {
				if (!hasUnitless) {
					startUnitless = animCurve.unitlessInput (0);
					endUnitless = animCurve.unitlessInput (numKeys - 1);
					hasUnitless = true;
				}
				else {
					startUnitless = min (startUnitless, animCurve.unitlessInput (0));
					endUnitless = max (endUnitless, animCurve.unitlessInput (numKeys - 1));
				}
			}
			else {
				if (!hasTime) {
					startTime = animCurve.time (0).value();
					endTime = animCurve.time (numKeys - 1).value();
					hasTime = true;
				}
				else {
					startTime = min (startTime, animCurve.time (0).value());
					endTime = max (endTime, animCurve.time (numKeys - 1).value());
				}
			}
		}
	}

	// Write out the header information
	//
	animWriter writer;
	writer.writeHeader (animFile, startTime, endTime, startUnitless, endUnitless);

	// Now write out the animation for each object on the selection list
	//
	unsigned int numObjects = list.length();
	for (i = 0; i < numObjects; i++) {
		MDagPath path;
		MObject node;
		if (list.getDagPath (i, path) == MS::kSuccess) {
			write (animFile, path);
		}
		else if (list.getDependNode (i, node) == MS::kSuccess) {
			write (animFile, node);
		}
	}

	animFile.flush();
	animFile.close();

	return (MS::kSuccess);
}

void
TanimExportUtil::write (std::ofstream &animFile, const MDagPath &path)
{
	// Walk through the dag breadth first
	//
	MItDag dagIt (MItDag::kDepthFirst);
	dagIt.reset (path, MItDag::kDepthFirst);
	for (; !dagIt.isDone(); dagIt.next()) {
		MDagPath thisPath;
		if (dagIt.getPath (thisPath) != MS::kSuccess) {
			continue;
		}
		// Find the animated plugs for this object
		//
		MPlugArray animatedPlugs;
		MObject node = thisPath.node();
		MFnDependencyNode fnNode (node);
		MAnimUtil::findAnimatedPlugs (thisPath, animatedPlugs);
		unsigned int numPlugs = animatedPlugs.length();
		if (numPlugs == 0) {
			// If the object is not animated, then write out place holder
			// information
			//
			animFile << "anim " << fnNode.name().asChar() << " " << dagIt.depth() << " " << thisPath.childCount() << " 0;\n";
		}
		else {
			// Otherwise write out each animation curve
			//
			writeAnimatedPlugs (animFile, animatedPlugs, fnNode.name(), dagIt.depth(), thisPath.childCount());
		}
	}
}

void
TanimExportUtil::write (std::ofstream &animFile, const MObject &node)
{
	// Watch out for characters and handle them a little differently
	//
	if (node.hasFn (MFn::kCharacter)) {
		MObjectArray characterList;
		characterList.append (node);
		MIntArray depths;
		depths.append (0);
		unsigned int current = 0;
		while (current < characterList.length()) {
			const MObject &thisNode = characterList[current];
			int thisDepth = depths[current++];
			// If this node is a character, then check for any immediate
			// subCharacters
			//
			MFnSet fnSet (thisNode);
			// Now find the set members
			//
			MSelectionList members;
			fnSet.getMembers (members, false);
			unsigned int childCount = 0;
			MItSelectionList iter (members, MFn::kCharacter);
			for (; !iter.isDone(); iter.next()) {
				MObject childNode;
				iter.getDependNode (childNode);
				characterList.insert (childNode, current + childCount);
				depths.insert (thisDepth + 1, current + childCount);
				childCount++;
			}
			// Find the animated plugs for this object
			//
			MPlugArray animatedPlugs;
			MAnimUtil::findAnimatedPlugs (thisNode, animatedPlugs);
			unsigned int numPlugs = animatedPlugs.length();
			if (numPlugs == 0) {
				// If the object is not animated, then write out place holder
				// information
				//
				animFile << "anim " << fnSet.name().asChar() << " " << thisDepth << " " << childCount << " 0;\n";
			}
			else {
				// Otherwise write out each animation curve
				//
				writeAnimatedPlugs (animFile, animatedPlugs, fnSet.name(), thisDepth, childCount);
			}
		}
		return;
	}
	// Find the animated plugs for this object
	//
	MPlugArray animatedPlugs;
	MFnDependencyNode fnNode (node);
	MAnimUtil::findAnimatedPlugs (node, animatedPlugs);
	unsigned int numPlugs = animatedPlugs.length();
	if (numPlugs != 0) {
		// If the object is animated the write out each animation curve
		//
		writeAnimatedPlugs (animFile, animatedPlugs, fnNode.name(), 0, 0);
	}
}

void
TanimExportUtil::writeAnimatedPlugs (
    std::ofstream &animFile,
	const MPlugArray &animatedPlugs,
	const MString &nodeName,
	unsigned int depth,
	unsigned int childCount
)
{
	// Walk through each animated plug and write out the animation curve(s)
	//
	unsigned int numPlugs = animatedPlugs.length();
	for (unsigned int i = 0; i < numPlugs; i++) {
		MPlug plug = animatedPlugs[i];
		MObjectArray animation;
		if (!MAnimUtil::findAnimation (plug, animation)) {
			continue;
		}
		// Write out the plugs' anim statement
		//
		animFile << "anim ";
		// build up the full attribute name
		//
		MPlug attrPlug (plug);
		MObject attrObj = attrPlug.attribute();
		MFnAttribute fnAttr (attrObj);
		MString fullAttrName (fnAttr.name());
		attrPlug = attrPlug.parent();
		while (!attrPlug.isNull()) {
			attrObj = attrPlug.attribute();
			MFnAttribute fnAttr2 (attrObj);
			fullAttrName = fnAttr2.name() + "." + fullAttrName;
			attrPlug = attrPlug.parent();
		}
		attrObj = plug.attribute();
		MFnAttribute fnLeafAttr (attrObj);
		animFile << fullAttrName.asChar() << " " << fnLeafAttr.name().asChar() << " " << nodeName.asChar() << " " << depth << " " << childCount << " " << i << ";\n";
		unsigned int numCurves = animation.length();
		// Write out each animation curve that animates the plug
		//
		for (unsigned int j = 0; j < numCurves; j++) {
			MObject animCurveNode = animation[j];
			if (!animCurveNode.hasFn (MFn::kAnimCurve)) {
				continue;
			}
			animWriter writer;
			writer.writeAnimCurve (animFile, &animCurveNode);
		}
	}
}

bool
TanimExportUtil::haveWriteMethod () const
{
	return (true);
}

MString
TanimExportUtil::defaultExtension () const
{
	return (MString("anim"));
}

MPxFileTranslator::MFileKind
TanimExportUtil::identifyFile (
	const MFileObject &file,
	const char * /*buffer*/,
	short /*size*/
) const
{
	const char *name = file.resolvedName().asChar();
	int   nameLength = (int)strlen(name);

	if ((nameLength > 5) && !strcasecmp(name+nameLength-5, ".anim")) {
		return (kIsMyFileType);
	}

	return (kNotMyFileType);
}

/* static */ void *
TanimExportUtil::creator ()
{
	return (new TanimExportUtil);
}

//--------------------------------------------------------------------------------
//	Plugin management
//--------------------------------------------------------------------------------

MStatus
initializePlugin (MObject obj)
{
	MStatus status;
	MFnPlugin plugin (obj, PLUGIN_COMPANY, "3.0");
	status = plugin.registerFileTranslator ("animExportUtil", "", TanimExportUtil::creator);
	return (status);
}

MStatus
uninitializePlugin (MObject obj)
{
	MFnPlugin plugin (obj);
	plugin.deregisterFileTranslator ("animExportUtil");
	return (MS::kSuccess);
}
