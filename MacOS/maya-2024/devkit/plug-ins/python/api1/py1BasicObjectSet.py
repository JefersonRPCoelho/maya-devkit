#-
# ==========================================================================
# Copyright (C) 1995 - 2006 Autodesk, Inc. and/or its licensors.  All 
# rights reserved.
#
# The coded instructions, statements, computer programs, and/or related 
# material (collectively the "Data") in these files contain unpublished 
# information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
# licensors, which is protected by U.S. and Canadian federal copyright 
# law and by international treaties.
#
# The Data is provided for use exclusively by You. You have the right 
# to use, modify, and incorporate this Data into other products for 
# purposes authorized by the Autodesk software license agreement, 
# without fee.
#
# The copyright notices in the Software and this entire statement, 
# including the above license grant, this restriction and the 
# following disclaimer, must be included in all copies of the 
# Software, in whole or in part, and all derivative works of 
# the Software, unless such copies or derivative works are solely 
# in the form of machine-executable object code generated by a 
# source language processor.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
# AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
# WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
# NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR 
# PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE, OR 
# TRADE PRACTICE. IN NO EVENT WILL AUTODESK AND/OR ITS LICENSORS 
# BE LIABLE FOR ANY LOST REVENUES, DATA, OR PROFITS, OR SPECIAL, 
# DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK 
# AND/OR ITS LICENSORS HAS BEEN ADVISED OF THE POSSIBILITY 
# OR PROBABILITY OF SUCH DAMAGES.
#
# ==========================================================================
#+

# Creation Date:   12 October 2006

########################################################################
# DESCRIPTION:
#
# Produces an objectSet node "spBasicObjectSet".
# 
# This plug-in implements a proxy objectSet node and a command for adding
# selected elements to a newly created spBasicObjectSet.
# 
# To create one of these nodes, enter the following commands after the plug-in
# is loaded:
#
#	import maya.cmds as cmds
#	cmds.createNode("spBasicObjectSet")
#
# An example script basicObjectSetTest.py is supplied in the developer kit.
# This example adds and removes objects from a spBasicObjectSet node. 
#
########################################################################

import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import math, sys

kNodeName = "spBasicObjectSet"
kCmdName = "spBasicObjectSetTest"
kNodeId = OpenMaya.MTypeId(0x00080065)

# Node definition
class BasicObjectSet(OpenMayaMPx.MPxObjectSet):
	def __init__(self):
		OpenMayaMPx.MPxObjectSet.__init__(self)


class BasicObjectSetTest(OpenMayaMPx.MPxCommand):
	def __init__(self):
		OpenMayaMPx.MPxCommand.__init__(self)
		self.__fDGMod = OpenMaya.MDGModifier()


	def doIt(self, args):
		# Create the node
		#
		setNode = self.__fDGMod.createNode(kNodeId)
		self.__fDGMod.doIt()

		# Populate the set with the selected items
		#
		selList = OpenMaya.MSelectionList()
		OpenMaya.MGlobal.getActiveSelectionList(selList)
		if selList.length():
			setFn = OpenMaya.MFnSet(setNode)
			setFn.addMembers(selList)

		depNodeFn = OpenMaya.MFnDependencyNode(setNode)
		OpenMayaMPx.MPxCommand.setResult(depNodeFn.name())


#####################################################################


# creator
def nodeCreator():
	return OpenMayaMPx.asMPxPtr(BasicObjectSet())


# initializer
def nodeInitializer():
	# nothing to initialize
	pass


def cmdCreator():
	return OpenMayaMPx.asMPxPtr(BasicObjectSetTest())


def cmdSyntaxCreator():
	return OpenMaya.MSyntax()


# initialize the script plug-in
def initializePlugin(mobject):
	mplugin = OpenMayaMPx.MFnPlugin(mobject, "Autodesk", "1.0", "Any")
	try:
		mplugin.registerCommand(kCmdName, cmdCreator, cmdSyntaxCreator)
	except:
		sys.stderr.write("Failed to register command: %s" % kCmdName)
		raise

	try:
		mplugin.registerNode(kNodeName, kNodeId, nodeCreator, nodeInitializer, OpenMayaMPx.MPxNode.kObjectSet)
	except:
		sys.stderr.write("Failed to register node: %s" % kNodeName)
		raise


# uninitialize the script plug-in
def uninitializePlugin(mobject):
	mplugin = OpenMayaMPx.MFnPlugin(mobject)
	try:
		mplugin.deregisterCommand(kCmdName)
	except:
		sys.stderr.write("Failed to deregister command: %s" % kCmdName)
		raise

	try:
		mplugin.deregisterNode(kNodeId)
	except:
		sys.stderr.write("Failed to deregister node: %s" % kNodeName)
		raise