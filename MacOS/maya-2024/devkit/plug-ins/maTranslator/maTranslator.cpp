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
// Produces the file translator Maya ASCII (via plug-in).
//
// This plugin is an example of a file translator.  Although, this is not
// the actual code used by Maya when it creates files in MayaAscii format,
// it nonetheless produces a very close approximation of the of that same
// format.  Close enough that Maya can load the resulting files as if they
// were MayaAscii.
//
// Currently, the plugin does not support the following:
//
//   o  Export Selection.  The plugin will only export entire scenes.
//
//   o  Referencing files into the default namespace, or using a renaming
//      prefix.  It only supports referencing files into a separate
//      namespace.
//
//   o  MEL reference files.
//
//   o  Size hints for multi plugs.
//
// To use this plug-in, load it and then invoke it through the Export All menu item.
//
////////////////////////////////////////////////////////////////////////

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFileIO.h>
#include <maya/MFileObject.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

#include <ctype.h>
#include <time.h>

#include <fstream>
#include <ios>

class maTranslator : public MPxFileTranslator
{
public:
	bool		haveReadMethod() const override;
	bool		haveWriteMethod() const override;
	MString		defaultExtension() const override;

	MFileKind	identifyFile(
					const MFileObject& file,
					const char* buffer,
					short size
				) const override;

	MStatus		reader(
					const MFileObject& file,
					const MString& options,
					FileAccessMode mode
				) override;

	MStatus		writer(
					const MFileObject& file,
					const MString& options,
					FileAccessMode mode
				) override;

	static void*	creator();
	static void		setPluginName(const MString& name);
	static MString	translatorName();

protected:
	void	getAddAttrCmds(const MObject& node, MStringArray& cmds);
	void	getSetAttrCmds(const MObject& node, MStringArray& cmds);
	void	writeBrokenRefConnections(std::fstream& f);
	void	writeConnections(std::fstream& f);
	void	writeCreateNode(std::fstream& f, const MObject& node);

	void	writeCreateNode(
				std::fstream& f, const MDagPath& nodePath, const MDagPath& parentPath
			);

	void	writeDagNodes(std::fstream& f);
	void	writeDefaultNodes(std::fstream& f);
	void	writeFileInfo(std::fstream& f);
	void	writeFooter(std::fstream& f, const MString& fileName);
	void	writeHeader(std::fstream& f, const MString& fileName);
	void	writeInstances(std::fstream& f);
	void	writeLockNode(std::fstream& f, const MObject& node);
	void	writeNodeAttrs(std::fstream& f, const MObject& node, bool isSelected);
	void	writeNodeConnections(std::fstream& f, const MObject& node);
	void	writeNonDagNodes(std::fstream& f);

	void	writeParent(
				std::fstream& f,
				const MDagPath& parent,
				const MDagPath& child,
				bool addIt
			);

	void	writePlugSizeHint(std::fstream& f, const MPlug& plug);
	void	writeReferences(std::fstream& f);
	void	writeReferenceNodes(std::fstream& f);
	void	writeRefNodeParenting(std::fstream& f);
	void	writeRequirements(std::fstream& f);
	void	writeSelectNode(std::fstream& f, const MObject& node);
	void	writeUnits(std::fstream& f);

	static MString	comment(const MString& text);
	static MString	quote(const MString& text);

	static MString	fExtension;
	static MString	fFileVersion;
	static MString	fPluginName;
	static MString	fTranslatorName;

private:
	//
	// These are used to keep track of connections which were made within
	// a referenced file but then broken by main scene file.
	//
	MPlugArray		fBrokenConnSrcs;
	MPlugArray		fBrokenConnDests;

	//
	// This is used to keep track of default nodes.
	//
	MObjectArray	fDefaultNodes;

	//
	// These are used to keep track of those DAG nodes which have multiple
	// instances.  'fInstanceParents' holds the first parent, which is
	// usually set up when the child is created.
	//
	MDagPathArray	fInstanceChildren;
	MDagPathArray	fInstanceParents;

	//
	// This is used to keep track of non-reference nodes with referenced
	// parents and referenced nodes with non-referenced parents, as their
	// parenting requires special handling.
	//
	MDagPathArray	fParentingRequired;

	//
	// These are used to store the IDs of the temporary node flags used by 
	// the translator.
	//
	unsigned int	fAttrFlag;
	unsigned int	fCreateFlag;
	unsigned int	fConnectionFlag;
};

//
// Note that this translator writes out 4.5ff01 version Maya ASCII
// files, regardless of the current Maya version.
//
MString maTranslator::fFileVersion = "4.5ff01";
MString maTranslator::fExtension = "pma";
MString maTranslator::fPluginName = "";
MString maTranslator::fTranslatorName = "Maya ASCII (via plugin)";


inline MString maTranslator::defaultExtension() const
{	return fExtension;		}


inline bool maTranslator::haveReadMethod() const
{	return false;			}


inline bool maTranslator::haveWriteMethod() const
{	return true;			}


inline void maTranslator::setPluginName(const MString& name)
{	fPluginName = name;		}


inline MString maTranslator::translatorName()
{	return fTranslatorName;	}


void* maTranslator::creator()
{
	return new maTranslator();
}


//
// Maya calls this method to find out if this translator is capable of
// handling the given file.
//
MPxFileTranslator::MFileKind maTranslator::identifyFile(
		const MFileObject& file, const char* buffer, short bufferLen
) const
{
	MString	tagStr = comment(fTranslatorName);
	int		tagLen = tagStr.length();

	//
	// If the buffer contains enough info to positively identify the file,
	// then use it.  Otherwise we'll base the identification on the file
	// extension.
	//
	if (bufferLen >= tagLen)
	{
		MString	initialContents(buffer, bufferLen);
		MStringArray	initialLines;

		initialContents.split('\n', initialLines);

		if (initialLines.length() > 0)
		{
			if (((int)initialLines[0].length() >= tagLen)
			&&	(initialLines[0].substring(0, tagLen-1) == tagStr))
			{
				return kIsMyFileType;
			}
		}
	}
	else
	{
		MString	fileName(file.resolvedName());
		int		fileNameLen = fileName.length();
		int		startOfExtension = fileName.rindex('.') + 1;

		if ((startOfExtension > 0)
		&&	(startOfExtension < fileNameLen)
		&&	(fileName.substring(startOfExtension, fileNameLen) == fExtension))
		{
			return kIsMyFileType;
		}
	}

	return kNotMyFileType;
}


//
// Maya calls this method to have the translator write out a file.
//
MStatus maTranslator::writer(
		const MFileObject& file,
		const MString& /* options */,
		MPxFileTranslator::FileAccessMode mode
)
{
	//
	// For simplicity, we only do full saves/exports.
	//
	if ((mode != kSaveAccessMode) && (mode != kExportAccessMode))
	   	return MS::kNotImplemented;

	//
	// Let's see if we can open the output file.
	//
	std::fstream output(file.expandedFullName().asChar(), std::ios::out | std::ios::trunc);

	if (!output.good()) return MS::kNotFound;

	//
	// Get some node flags to keep track of those nodes for which we
	// have already done various stages of processing.
	//
	MStatus	status;

	fCreateFlag = MFnDependencyNode::allocateFlag(fPluginName, &status);

	if (status)
		fAttrFlag = MFnDependencyNode::allocateFlag(fPluginName, &status);

	if (status)
		fConnectionFlag = MFnDependencyNode::allocateFlag(fPluginName, &status);

	if (!status)
	{
		MGlobal::displayError(
			"Could not allocate three free node flags."
			"  Try unloading some other plugins."
		);

		return MS::kFailure;
	}

	//
	// Run through all of the nodes in the scene and clear their flags.
	//
	MItDependencyNodes	nodesIter;

	for (; !nodesIter.isDone(); nodesIter.next())
	{
		MObject				node = nodesIter.thisNode();
		MFnDependencyNode	nodeFn(node);

		nodeFn.setFlag(fCreateFlag, false);
		nodeFn.setFlag(fAttrFlag, false);
		nodeFn.setFlag(fConnectionFlag, false);
	}

	//
	// Write out the various sections of the file.
	//
	writeHeader(output, file.resolvedName());
	writeFileInfo(output);
	writeReferences(output);
	writeRequirements(output);
	writeUnits(output);
	writeDagNodes(output);
	writeNonDagNodes(output);
	writeDefaultNodes(output);
	writeReferenceNodes(output);
	writeConnections(output);
	writeFooter(output, file.resolvedName());

	output.close();

	MFnDependencyNode::deallocateFlag(fPluginName, fCreateFlag);

	return MS::kSuccess;
}


void maTranslator::writeHeader(std::fstream& f, const MString& fileName)
{
	//
	// Get the current time into the same format as used by Maya ASCII
	// files.
	//
	time_t		tempTime = time(NULL);
	struct tm*	curTime = localtime(&tempTime);
	char		formattedTime[100];

	strftime(
		formattedTime, sizeof(formattedTime), "%a, %b %e, %Y %r", curTime
	);

	//
	// Write out the header information.
	//
	f << comment(fTranslatorName).asChar() << " "
		<< fFileVersion.asChar() << " scene" << std::endl;
	f << comment("Name: ").asChar() << fileName.asChar() << std::endl;
	f << comment("Last modified: ").asChar() << formattedTime << std::endl;
}


//
// Write out the "fileInfo" command for the freeform information associated
// with the scene.
//
void maTranslator::writeFileInfo(std::fstream& f)
{
	//
	// There's no direct access to the scene's fileInfo from within the API,
	// so we have to call MEL's 'fileInfo' command.
	//
	MStringArray	fileInfo;

	if (MGlobal::executeCommand("fileInfo -q", fileInfo))
	{
		unsigned	numEntries = fileInfo.length();
		unsigned	i;

		for (i = 0; i < numEntries; i += 2)
		{
			f << "fileInfo " << quote(fileInfo[i]).asChar() << " "
					<< quote(fileInfo[i+1]).asChar() << ";" << std::endl;
		}
	}
	else
		MGlobal::displayWarning("Could not get scene's fileInfo.");
}


//
// Write out the "file" commands which specify the reference files used by
// the scene.
//
void maTranslator::writeReferences(std::fstream& f)
{
	MStringArray	files;

	MFileIO::getReferences(files);

	unsigned	numRefs = files.length();
	unsigned	i;

	for (i = 0; i < numRefs; i++)
	{
		MString	refCmd = "file -r";
		MString	fileName = files[i];
		MString	nsName = "";

		//
		// For simplicity, we assume that namespaces are always used when
		// referencing.
		//
		MString tempCmd = "file -q -ns \"";
		tempCmd += fileName + "\"";

		if (MGlobal::executeCommand(tempCmd, nsName))
		{
			refCmd += " -ns \"";
			refCmd += nsName + "\"";
		}
		else
			MGlobal::displayWarning("Could not get namespace name.");

		//
		// Is this a deferred reference?
		//
		tempCmd = "file -q -dr \"";
		tempCmd += fileName + "\"";

		int	isDeferred;

		if (MGlobal::executeCommand(tempCmd, isDeferred))
		{
			if (isDeferred) refCmd += " -dr 1";
		}
		else
			MGlobal::displayWarning("Could not get deferred reference info.");

		//
		// Get the file's reference node, if it has one.
		//
		tempCmd = "file -q -rfn \"";
		tempCmd += fileName + "\"";

		MString	refNode;

		if (MGlobal::executeCommand(tempCmd, refNode))
		{
			if (refNode.length() > 0)
			{
				refCmd += " -rfn \"";
				refCmd += refNode + "\"";
			}
		}
		else
			MGlobal::displayInfo("Could not query reference node name.");

		//
		// Write out the reference command.
		//
		f << refCmd.asChar() << " \"" << fileName.asChar() << "\";" << std::endl;
	}
}


//
// Write out the "requires" lines which specify the plugins needed by the
// scene.
//
void maTranslator::writeRequirements(std::fstream& f)
{
	//
	// Every scene requires Maya itself.
	//
	f << "requires maya \"" << fFileVersion.asChar() << "\";" << std::endl;

	//
	// Write out requirements for each plugin.
	//
	MStringArray	pluginsUsed;

	if (MGlobal::executeCommand("pluginInfo -q -pluginsInUse", pluginsUsed))
	{
		unsigned	numPlugins = pluginsUsed.length();
		unsigned	i;

		for (i = 0; i < numPlugins; i += 2)
		{
			f << "requires " << quote(pluginsUsed[i]).asChar() << " "
					<< quote(pluginsUsed[i+1]).asChar() << ";" << std::endl;
		}
	}
	else
	{
		MGlobal::displayWarning(
			"Could not get list of plugins currently in use."
		);
	}
}


//
// Write out the units of measurement currently being used by the scene.
//
void maTranslator::writeUnits(std::fstream& f)
{
	MString	args = "";
	MString	result;

	//
	// Linear units.
	//
	if (MGlobal::executeCommand("currentUnit -q -fullName -linear", result))
		args += " -l " + result;
	else
		MGlobal::displayWarning("Could not get current linear units.");

	//
	// Angular units.
	//
	if (MGlobal::executeCommand("currentUnit -q -fullName -angle", result))
		args += " -a " + result;
	else
		MGlobal::displayWarning("Could not get current linear units.");

	//
	// Time units.
	//
	if (MGlobal::executeCommand("currentUnit -q -fullName -time", result))
		args += " -t " + result;
	else
		MGlobal::displayWarning("Could not get current linear units.");

	if (args != "")
	{
		f << "currentUnit" << args.asChar() << ";" << std::endl;
	}
}


void maTranslator::writeDagNodes(std::fstream& f)
{
	fParentingRequired.clear();

	MItDag		dagIter;

	dagIter.traverseUnderWorld(true);

	MDagPath	worldPath;

	dagIter.getPath(worldPath);

	//
	// We step over the world node before starting the loop, because it
	// doesn't get written out.
	//
	for (dagIter.next(); !dagIter.isDone(); dagIter.next())
	{
		MDagPath	path;
		dagIter.getPath(path);

		//
		// If the node has already been written, then all of its descendants
		// must have been written, or at least checked, as well, so prune
		// this branch of the tree from the iteration.
		//
		MFnDagNode	dagNodeFn(path);

		if (dagNodeFn.isFlagSet(fCreateFlag))
		{
			dagIter.prune();
			continue;
		}

		//
		// If this is a default node, it will be written out later, so skip
		// it.
		//
		if (dagNodeFn.isDefaultNode()) continue;

		//
		// If this node is not writable, and is not a shared node, then mark
		// it as having been written, and skip it.
		//
		if (!dagNodeFn.canBeWritten() && !dagNodeFn.isShared())
		{
			dagNodeFn.setFlag(fCreateFlag, true);
			continue;
		}

		unsigned int	numParents = dagNodeFn.parentCount();

		if (dagNodeFn.isFromReferencedFile())
		{
			//
			// We don't issue 'creatNode' commands for nodes from referenced
			// files, but if the node has any parents which are not from
			// referenced files, other than the world, then make a note that
			// we'll need to issue extra 'parent' commands for it later on.
			//
			unsigned int i;

			for (i = 0; i < numParents; i++)
			{
				MObject		altParent = dagNodeFn.parent(i);
				MFnDagNode	altParentFn(altParent);

				if (!altParentFn.isFromReferencedFile()
				&&	(altParentFn.object() != worldPath.node()))
				{
					fParentingRequired.append(path);
					break;
				}
			}
		}
		else
		{
			//
			// Find the node's parent.
			//
			MDagPath	parentPath = worldPath;

			if (path.length() > 1)
			{
				//
				// Get the parent's path.
				//
				parentPath = path;
				parentPath.pop();

				//
				// If the parent is in the underworld, then find the closest
				// ancestor which is not.
				//
				if (parentPath.pathCount() > 1)
				{
					//
					// The first segment of the path contains whatever
					// portion of the path exists in the world.  So the closest
					// worldly ancestor is simply the one at the end of that
					// first path segment.
					//
					path.getPath(parentPath, 0);
				}
			}

			MFnDagNode	parentNodeFn(parentPath);

			if (parentNodeFn.isFromReferencedFile())
			{
				//
				// We prefer to parent to a non-referenced node.  So if this
				// node has any other parents, which are not from referenced
				// files and have not already been processed, then we'll
				// skip this instance and wait for an instance through one
				// of those parents.
				//
				unsigned i;

				for (i = 0; i < numParents; i++)
				{
					if (dagNodeFn.parent(i) != parentNodeFn.object())
					{
						MObject		altParent = dagNodeFn.parent(i);
						MFnDagNode	altParentFn(altParent);

						if (!altParentFn.isFromReferencedFile()
						&&	!altParentFn.isFlagSet(fCreateFlag))
						{
							break;
						}
					}
				}

				if (i < numParents) continue;

				//
				// This node only has parents within referenced files, so
				// create it without a parent and note that we need to issue
				// 'parent' commands for it later on.
				//
				writeCreateNode(f, path, worldPath);

				fParentingRequired.append(path);
			}
			else
			{
				writeCreateNode(f, path, parentPath);

				//
				// Let's see if this node has any parents from referenced
				// files, or any parents other than this one which are not
				// from referenced files.
				//
				unsigned	int i;
				bool		hasRefParents = false;
				bool		hasOtherNonRefParents = false;

				for (i = 0; i < numParents; i++)
				{
					if (dagNodeFn.parent(i) != parentNodeFn.object())
					{
						MObject		altParent = dagNodeFn.parent(i);
						MFnDagNode	altParentFn(altParent);

						if (altParentFn.isFromReferencedFile())
							hasRefParents = true;
						else
							hasOtherNonRefParents = true;

						//
						// If we've already got positives for both tests,
						// then there's no need in continuing.
						//
						if (hasRefParents && hasOtherNonRefParents) break;
					}
				}

				//
				// If this node has parents from referenced files, then
				// make note that we will have to issue 'parent' commands
				// later on.
				//
				if (hasRefParents) fParentingRequired.append(path);

				//
				// If this node has parents other than this one which are
				// not from referenced files, then make note that the
				// parenting for the other instances still has to be done.
				//
				if (hasOtherNonRefParents)
				{
					fInstanceChildren.append(path);
					fInstanceParents.append(parentPath);
				}
			}

			//
			// Write out the node's 'addAttr', 'setAttr' and 'lockNode'
			// commands.
			//
			writeNodeAttrs(f, path.node(), true);
			writeLockNode(f, path.node());
		}

		//
		// Mark the node as having been written.
		//
		dagNodeFn.setFlag(fCreateFlag, true);
	}

	//
	// Write out the parenting for instances.
	//
	writeInstances(f);
}


//
// If a DAG node is instanced (i.e. has multiple parents), this method
// will put it under its remaining parents.  It will already have been put
// under its first parent when it was created.
//
void maTranslator::writeInstances(std::fstream& f)
{
	unsigned int numInstancedNodes = fInstanceChildren.length();
	unsigned int i;

	for (i = 0; i < numInstancedNodes; i++)
	{
		MFnDagNode	nodeFn(fInstanceChildren[i]);

		unsigned int numParents = nodeFn.parentCount();
		unsigned int p;

		for (p = 0; p < numParents; p++)
		{
			//
			// We don't want to issue a 'parent' command for the node's
			// existing parent.
			//
			if (nodeFn.parent(i) != fInstanceParents[i].node())
			{
				MObject		parent = nodeFn.parent(i);
				MFnDagNode	parentFn(parent);

				if (!parentFn.isFromReferencedFile())
				{
					//
					// Get the first path to the parent node.
					//
					MDagPath	parentPath;

					MDagPath::getAPathTo(parentFn.object(), parentPath);

					writeParent(f, parentPath, fInstanceChildren[i], true);
				}
			}
		}
	}

	//
	// We don't need this any more, so free up the space.
	//
	fInstanceChildren.clear();
	fInstanceParents.clear();
}


//
// Write out a 'parent' command to parent one DAG node under another.
//
void maTranslator::writeParent(
		std::fstream& f, const MDagPath& parent, const MDagPath& child, bool addIt
)
{
	f << "parent -s -nc -r ";
 
	//
	// If this is not the first parent then we have to include the "-a/add"
	// flag.
	//
	if (addIt) f << "-a ";

	//
	// If the parent is the world, then we must include the "-w/world" flag.
	//
	if (parent.length() == 0) f << "-w ";

	f << "\"" << child.partialPathName().asChar() << "\"";

	//
	// If the parent is NOT the world, then give the parent's name.
	//
	if (parent.length() != 0)
		f << " \"" << parent.partialPathName().asChar() << "\"";

	f << ";" << std::endl;
}


void maTranslator::writeNonDagNodes(std::fstream& f)
{
	MItDependencyNodes	nodeIter;

	for (; !nodeIter.isDone(); nodeIter.next())
	{
		MObject				node = nodeIter.thisNode();
		MFnDependencyNode	nodeFn(node);

		//
		// Save default nodes for later processing.
		//
		if (nodeFn.isDefaultNode())
		{
			fDefaultNodes.append(node);
		}
		else if (!nodeFn.isFromReferencedFile()
		&&	!nodeFn.isFlagSet(fCreateFlag))
		{
			//
			// If this node is either writable or shared, then write it out.
			// Otherwise don't, but still mark it as having been written so
			// that we don't end up processing it again at some later time.
			//
			if (nodeFn.canBeWritten() || nodeFn.isShared())
			{
				writeCreateNode(f, node);
				writeNodeAttrs(f, node, true);
				writeLockNode(f, node);
			}

			nodeFn.setFlag(fCreateFlag, true);
			nodeFn.setFlag(fAttrFlag, true);
		}
	}
}


void maTranslator::writeDefaultNodes(std::fstream& f)
{
	//
	// For default nodes we don't write out a createNode statement, but we
	// still write added attributes and changed attribute values.
	//
	unsigned int	numNodes = fDefaultNodes.length();
	unsigned int	i;

	for (i = 0; i < numNodes; i++)
	{
		writeNodeAttrs(f, fDefaultNodes[i], false);

		MFnDependencyNode	nodeFn(fDefaultNodes[i]);

		nodeFn.setFlag(fAttrFlag, true);
	}
}


//
// Write out the 'addAttr' and 'setAttr' commands for a node.
//
void maTranslator::writeNodeAttrs(
		std::fstream& f, const MObject& node, bool isSelected
)
{
	MFnDependencyNode	nodeFn(node);

	if (nodeFn.canBeWritten())
	{
		MStringArray	addAttrCmds;
		MStringArray	setAttrCmds;

		getAddAttrCmds(node, addAttrCmds);
		getSetAttrCmds(node, setAttrCmds);

		unsigned int	numAddAttrCmds = addAttrCmds.length();
		unsigned int	numSetAttrCmds = setAttrCmds.length();

		if (numAddAttrCmds + numSetAttrCmds > 0)
		{
			//
			// If the node is not already selected, then issue a command to
			// select it.
			//
			if (!isSelected) writeSelectNode(f, node);

			unsigned int i;

			for (i = 0; i < numAddAttrCmds; i++)
				f << addAttrCmds[i].asChar() << std::endl;

			for (i = 0; i < numSetAttrCmds; i++)
				f << setAttrCmds[i].asChar() << std::endl;
		}
	}
}


void maTranslator::writeReferenceNodes(std::fstream& f)
{
	//
	// We don't write out createNode commands for reference nodes, but
	// we do write out parenting between them and non-reference nodes,
	// as well as attributes added and attribute values changed after the
	// referenced file was loaded
	//
	writeRefNodeParenting(f);

	//
	// Output the commands for DAG nodes first.
	//
	MItDag	dagIter;

	for (dagIter.next(); !dagIter.isDone(); dagIter.next())
	{
		MObject				node = dagIter.currentItem();
		MFnDependencyNode	nodeFn(node);

		if (nodeFn.isFromReferencedFile()
		&&	!nodeFn.isFlagSet(fAttrFlag))
		{
			writeNodeAttrs(f, node, false);

			//
			// Make note of any connections to this node which have been
			// broken by the main scene.
			//
			MFileIO::getReferenceConnectionsBroken(
				node, fBrokenConnSrcs, fBrokenConnDests, true, true
			);

			nodeFn.setFlag(fAttrFlag, true);
		}
	}

	//
	// Now do the remaining, non-DAG nodes.
	//
	MItDependencyNodes	nodeIter;

	for (; !nodeIter.isDone(); nodeIter.next())
	{
		MObject				node = nodeIter.thisNode();
		MFnDependencyNode	nodeFn(node);

		if (nodeFn.isFromReferencedFile()
		&&	!nodeFn.isFlagSet(fAttrFlag))
		{
			writeNodeAttrs(f, node, false);

			//
			// Make note of any connections to this node which have been
			// broken by the main scene.
			//
			MFileIO::getReferenceConnectionsBroken(
				node, fBrokenConnSrcs, fBrokenConnDests, true, true
			);

			nodeFn.setFlag(fAttrFlag, true);
		}
	}
}


//
// Write out all of the connections in the scene.
//
void maTranslator::writeConnections(std::fstream& f)
{
	//
	// If the scene has broken any connections which were made in referenced
	// files, handle those first so that the attributes are free for any new
	// connections which may come along.
	//
	writeBrokenRefConnections(f);

	//
	// We're about to write out the scene's connections in three parts: DAG
	// nodes, non-DAG non-default nodes, then default nodes.
	//
	// It's really not necessary that we group them like this and would in
	// fact be more efficient to do them all in one MItDependencyNodes
	// traversal.  However, this is the order in which the normal MayaAscii
	// translator does them, so this makes it easier to compare the output
	// of this translator to Maya's output.
	//

	//
	// Write out connections for the DAG nodes first.
	//
	MItDag	dagIter;
	dagIter.traverseUnderWorld(true);

	for (dagIter.next(); !dagIter.isDone(); dagIter.next())
	{
		MObject		node = dagIter.currentItem();
		MFnDagNode	dagNodeFn(node);

		if (!dagNodeFn.isFlagSet(fConnectionFlag)
		&&	dagNodeFn.canBeWritten()
		&&	!dagNodeFn.isDefaultNode())
		{
			writeNodeConnections(f, dagIter.currentItem());
			dagNodeFn.setFlag(fConnectionFlag, true);
		}
	}

	//
	// Now do the non-DAG, non-default nodes.
	//
	MItDependencyNodes	nodeIter;

	for (; !nodeIter.isDone(); nodeIter.next())
	{
		MFnDependencyNode	nodeFn(nodeIter.thisNode());

		if (!nodeFn.isFlagSet(fConnectionFlag)
		&&	nodeFn.canBeWritten()
		&&	!nodeFn.isDefaultNode())
		{
			writeNodeConnections(f, nodeIter.thisNode());
			nodeFn.setFlag(fConnectionFlag, true);
		}
	}

	//
	// And finish up with the default nodes.
	//
	unsigned int	numNodes = fDefaultNodes.length();
	unsigned int	i;

	for (i = 0; i < numNodes; i++)
	{
		MFnDependencyNode	nodeFn(fDefaultNodes[i]);

		if (!nodeFn.isFlagSet(fConnectionFlag)
		&&	nodeFn.canBeWritten()
		&&	nodeFn.isDefaultNode())
		{
			writeNodeConnections(f, fDefaultNodes[i]);
			nodeFn.setFlag(fConnectionFlag, true);
		}
	}
}


//
// Write the 'disconnectAttr' statements for those connections which were
// made in referenced files, but broken in the main scene.
//
void maTranslator::writeBrokenRefConnections(std::fstream& f)
{
	unsigned int	numBrokenConnections = fBrokenConnSrcs.length();
	unsigned int	i;

	for (i = 0; i < numBrokenConnections; i++)
	{
		f << "disconnectAttr \""
		  << fBrokenConnSrcs[i].partialName(true).asChar()
		  << "\" \""
		  << fBrokenConnDests[i].partialName(true).asChar()
		  << "\"";

		//
		// If the destination plug is a multi for which index does not
		// matter, then we must add a "-na/nextAvailable" flag to the
		// command.
		//
		MObject			attr = fBrokenConnDests[i].attribute();
		MFnAttribute	attrFn(attr);

		if (!attrFn.indexMatters()) f << " -na";

		f << ";" << std::endl;
	}
}


//
// Write the 'connectAttr' commands for all of a node's incoming
// connections.
//
void maTranslator::writeNodeConnections(std::fstream& f, const MObject& node)
{
	MFnDependencyNode	nodeFn(node);
	MPlugArray			plugs;

	nodeFn.getConnections(plugs);

	unsigned int		numBrokenConns = fBrokenConnSrcs.length();
	unsigned int		numPlugs = plugs.length();
	unsigned int		i;

	for (i = 0; i < numPlugs; i++)
	{
		//
		// We only care about connections where we are the destination.
		//
		MPlug		destPlug = plugs[i];
		MPlugArray	srcPlug;

		destPlug.connectedTo(srcPlug, true, false);

		if (srcPlug.length() > 0)
		{
			MObject				srcNode = srcPlug[0].node();
			MFnDependencyNode	srcNodeFn(srcNode);

			//
			// Don't write the connection if the source is not writable...
			//
			if (!srcNodeFn.canBeWritten()) continue;

			//
			// or the connection was made in a referenced file...
			//
			if (destPlug.isFromReferencedFile()) continue;

			//
			// or the plug is procedural...
			//
			if (destPlug.isProcedural()) continue;

			//
			// or it is a connection between a default node and a shared
			// node (because those will get set up automatically).
			//
			if (srcNodeFn.isDefaultNode() && nodeFn.isShared()) continue;

			f << "connectAttr \"";

			//
			// Default nodes get a colon at the start of their names.
			//
			if (srcNodeFn.isDefaultNode()) f << ":";

			f << srcPlug[0].partialName(true).asChar()
			  << "\" \"";

			if (nodeFn.isDefaultNode()) f << ":";

			f << destPlug.partialName(true).asChar()
			  << "\"";

			//
			// If the src plug is also one from which a broken
			// connection originated, then add the "-rd/referenceDest" flag
			// to the command.  That will help Maya to better adjust if the
			// referenced file has changed the next time it is loaded.
			//
			if (srcNodeFn.isFromReferencedFile())
			{
				unsigned int j;

				for (j = 0; j < numBrokenConns; j++)
				{
					if (fBrokenConnSrcs[j] == srcPlug[0])
					{
						f << " -rd \""
						  << fBrokenConnDests[j].partialName(true).asChar()
						  << "\"";

						break;
					}
				}
			}

			//
			// If the plug is locked, then add a "-l/lock" flag to the
			// command.
			//
			if (destPlug.isLocked()) f << " -l on";

			//
			// If the destination attribute is a multi for which index
			// does not matter, then we must add the "-na/nextAvailable"
			// flag to the command.
			//
			MObject			attr = destPlug.attribute();
			MFnAttribute	attrFn(attr);

			if (!attrFn.indexMatters()) f << " -na";

			f << ";" << std::endl;
		}
	}
}


//
// Write out a 'createNode' command for a DAG node.
//
void maTranslator::writeCreateNode(
		std::fstream& f, const MDagPath& nodePath, const MDagPath& parentPath
)
{
	MObject		node(nodePath.node());
	MFnDagNode	nodeFn(node);

	//
	// Write out the 'createNode' command for this node.
	//
	f << "createNode " << nodeFn.typeName().asChar();

	//
	// If the node is shared, then add a "-s/shared" flag to the command.
	//
	if (nodeFn.isShared()) f << " -s";

	f << " -n \"" << nodeFn.name().asChar() << "\"";

	//
	// If this is not a top-level node, then include its first parent in the
	// command.
	//
	if (parentPath.length() > 0)
		f << " -p \"" << parentPath.partialPathName().asChar() << "\"";
   
	f << ";" << std::endl;
}


//
// Write out a 'createNode' command for a non-DAG node.
//
void maTranslator::writeCreateNode(std::fstream& f, const MObject& node)
{
	MFnDependencyNode	nodeFn(node);

	//
	// Write out the 'createNode' command for this node.
	//
	f << "createNode " << nodeFn.typeName().asChar();

	//
	// If the node is shared, then add a "-s/shared" flag to the command.
	//
	if (nodeFn.isShared()) f << " -s";

	f << " -n \"" << nodeFn.name().asChar() << "\";" << std::endl;
}


//
// Write out a "lockNode" command.
//
void maTranslator::writeLockNode(std::fstream& f, const MObject& node)
{
	MFnDependencyNode	nodeFn(node);

	//
	// By default, nodes are not locked, so we only have to issue a
	// "lockNode" command if the node is locked.
	//
	if (nodeFn.isLocked()) f << "lockNode;" << std::endl;
}


//
// Write out a "select" command.
//
void maTranslator::writeSelectNode(std::fstream& f, const MObject& node)
{
	MStatus				status;
	MFnDependencyNode	nodeFn(node);
	MString				nodeName;

	//
	// If the node has a unique name, then we can just go ahead and use
	// that.  Otherwise we will have to use part of its DAG path to to
	// distinguish it from the others with the same name.
	//
	if (nodeFn.hasUniqueName())
		nodeName = nodeFn.name();
	else
	{
		//
		// Only DAG nodes are allowed to have duplicate names.
		//
		MFnDagNode	dagNodeFn(node, &status);

		if (!status)
		{
			MGlobal::displayWarning(
				MString("Node '") + nodeFn.name()
				+ "' has a non-unique name but claimes to not be a DAG node.\n"
				+ "Using non-unique name."
			);

			nodeName = nodeFn.name();
		}
		else
			nodeName = dagNodeFn.partialPathName();
	}

	//
	// We use the "-ne/noExpand" flag so that if the node is a set, we
	// actually select the set itself, rather than its members.
	//
	f << "select -ne ";

	//
	// Default nodes get a colon slapped onto the start of their names.
	//
	if (nodeFn.isDefaultNode()) f << ":";

	f << nodeName.asChar() << ";\n";
}


//
// Deal with nodes whose parenting is between referenced and non-referenced
// nodes.
//
void maTranslator::writeRefNodeParenting(std::fstream& f)
{
	unsigned int numNodes = fParentingRequired.length();
	unsigned int i;

	for (i = 0; i < numNodes; i++)
	{
		MFnDagNode	nodeFn(fParentingRequired[i]);

		//
		// Find out if this node has any parents from referenced or
		// non-referenced files.
		//
		bool			hasRefParents = false;
		bool			hasNonRefParents = false;
		unsigned int	numParents = nodeFn.parentCount();
		unsigned int	p;

		for (p = 0; p < numParents; p++)
		{
			MObject		parent = nodeFn.parent(p);
			MFnDagNode	parentFn(parent);

			if (parentFn.isFromReferencedFile())
				hasRefParents = true;
			else
				hasNonRefParents = true;

			if (hasRefParents && hasNonRefParents) break;
		}

		//
		// If this node is from a referenced file and it has parents which
		// are also from a referenced file, then it already has its first
		// parent and all others are added instances.
		//
		// Similarly if the node is not from a referenced file and has
		// parents which are also not from referenced files.
		//
		bool	alreadyHasFirstParent =
			(nodeFn.isFromReferencedFile() ? hasRefParents : hasNonRefParents);

		//
		// Now run through the parents again and output any parenting
		// which involves a non-referenced node, either as parent or child.
		//
		for (p = 0; p < numParents; p++)
		{
			MObject		parent = nodeFn.parent(p);
			MFnDagNode	parentFn(parent);

			if (parentFn.isFromReferencedFile() != nodeFn.isFromReferencedFile())
			{
				//
				// Get the first path to the parent.
				//
				MDagPath	parentPath;
				MDagPath::getAPathTo(parentFn.object(), parentPath);

				writeParent(
					f, parentPath, fParentingRequired[i], alreadyHasFirstParent
				);

				//
				// If it didn't have its first parent before, it does now.
				//
				alreadyHasFirstParent = true;
			}
		}
	}
}


void maTranslator::writeFooter(std::fstream& f, const MString& fileName)
{
	f << comment(" End of ").asChar() << fileName.asChar() << std::endl;
}


void maTranslator::getAddAttrCmds(const MObject& node, MStringArray& cmds)
{
	//
	// Run through the node's attributes.
	//
	MFnDependencyNode	nodeFn(node);
	unsigned int		numAttrs = nodeFn.attributeCount();
	unsigned int		i;

	for (i = 0; i < numAttrs; i++)
	{
		//
		// Use the attribute ordering which Maya uses when doing I/O.
		//
		MObject	attr = nodeFn.reorderedAttribute(i);

		//
		// If this attribute has been added since the node was created,
		// then we may want to write out an addAttr statement for it.
		//
		if (nodeFn.isNewAttribute(attr))
		{
			MFnAttribute	attrFn(attr);

			//
			// If the attribute has a parent then ignore it because it will
			// be processed when we process the parent.
			//
			MStatus	status;

			attrFn.parent(&status);

			if (status == MS::kNotFound)
			{
				//
				// If the attribute is a compound, then we can do its entire
				// tree at once.
				//
				MFnCompoundAttribute	cAttrFn(attr, &status);

				if (status)
				{
					MStringArray	newCmds;

					cAttrFn.getAddAttrCmds(newCmds);

					unsigned int	numCommands = newCmds.length();
					unsigned int	c;

					for (c = 0; c < numCommands; c++)
					{
						if (newCmds[c] != "")
							cmds.append(newCmds[c]);
					}
				}
				else
				{
					MString	newCmd = attrFn.getAddAttrCmd();

					if (newCmd != "") cmds.append(newCmd);
				}
			}
		}
	}
}


void maTranslator::getSetAttrCmds(const MObject& node, MStringArray& cmds)
{
	//
	// Get rid of any garbage already in the array.
	//
	cmds.clear();

	//
	// Run through the node's attributes.
	//
	MFnDependencyNode	nodeFn(node);
	unsigned int		numAttrs = nodeFn.attributeCount();
	unsigned int		i;

	for (i = 0; i < numAttrs; i++)
	{
		//
		// Use the attribute ordering which Maya uses when doing I/O.
		//
		MObject			attr = nodeFn.reorderedAttribute(i);
		MFnAttribute	attrFn(attr);
		MStatus			status;

		attrFn.parent(&status);

		bool			isChild = (status != MS::kNotFound);

		//
		// We don't want attributes which are children of other attributes
		// because they will be processed when we process the parent.
		//
		// And we only want storable attributes which accept inputs.
		//
		if (!isChild && attrFn.isStorable() && attrFn.isWritable())
		{
			//
			// Get a plug for the attribute.
			//
			MPlug	plug(node, attr);

			//
			// Get setAttr commands for this attribute, and any of its
			// children, which have had their values changed by the scene.
			//
			MStringArray	newCmds;

			plug.getSetAttrCmds(newCmds, MPlug::kChanged, false);

			unsigned int	numCommands = newCmds.length();
			unsigned int	c;

			for (c = 0; c < numCommands; c++)
			{
				if (newCmds[c] != "")
					cmds.append(newCmds[c]);
			}
		}
	}
}


MStatus maTranslator::reader(
		const MFileObject& /* file */,
		const MString& /* options */,
		MPxFileTranslator::FileAccessMode /* mode */
)
{
	return MS::kNotImplemented;
}


MString maTranslator::comment(const MString& text)
{
	MString	result("//");
	result += text;

	return result;
}


//
// Convert a string into a quoted, printable string.
//
MString maTranslator::quote(const MString& str)
{
	const char* cstr = str.asChar();
	int	strLen = str.length();
	int i;

	MString	result("\"");

	for (i = 0; i < strLen; i++)
	{
		int c = cstr[i];

		if (isprint(c))
		{
			//
			// Because backslash and double-quote have special meaning
			// within a printable string, we have to turn those into escape
			// sequences.
			//
			switch (c)
			{
				case '"':
					result += "\\\"";
				break;

				case '\\':
					result += "\\\\";
				break;

				default:
					result += MString((const char*)&c, 1);
				break;
			}
		}
		else
		{
			//
			// Convert non-printable characters into escape sequences.
			//
			switch (c)
			{
				case '\n':
					result += "\\n";
				break;

				case '\t':
					result += "\\t";
				break;

				case '\b':
					result += "\\b";
				break;

				case '\r':
					result += "\\r";
				break;

				case '\f':
					result += "\\f";
				break;

				case '\v':
					result += "\\v";
				break;

				case '\007':
					result += "\\a";
				break;

				default:
				{
					//
					// Encode it as an octal escape sequence.
					//
					char buff[5];
					sprintf(buff, "\\%.3o", c);
					result += MString(buff, 4);
				}
			}
		}
	}

	//
	// Add closing quote.
	//
	result += "\"";

	return result;
}


// ****************************************

MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "1.0", "Any");

	maTranslator::setPluginName(plugin.name());

	plugin.registerFileTranslator(
		maTranslator::translatorName(),
		NULL,
		maTranslator::creator,
		NULL,
		NULL,
		false
	);

	return MS::kSuccess;
}


MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin( obj );

	plugin.deregisterFileTranslator(maTranslator::translatorName());

	return MS::kSuccess;
}
