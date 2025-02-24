
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

########################################################################
# DESCRIPTION:
#
# Produces the dependency graph node "simpleSpring".
#
# This node is an example of a spring node that calculates the spring behavior,
# which Maya uses in a simulation.    
#
########################################################################

import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx

#
# To test out this plugin example:
# Run from a Python tab of the script editor.
#
#import maya.cmds as cmds
#
#cmds.file(force=True, new=True)
#cmds.currentUnit(linear="centimeter", angle="degree", time="film")   
#
## Load the plug-in.
##
#cmds.loadPlugin("simpleSpring.py")
#
## Create the spring node.
#cmds.createNode("spSimpleSpring", name="simpleSpring")
#
#cmds.curve(d=3, p=[(-9, 6, 0), (-4, 6, 0), (4, 6, 0), (9, 6, 0)], k=[(0), (0), (0), (1), (1), (1)])
#
#cmds.particle(n="fourParticles", p=[(11, -2, 0), (4, -2, 0), (-3, -2, 0), (-7, 4, 0)], c=1)
#
#cmds.gravity(pos=(0, 0, 0), m=9.8, dx=0, dy=-1, dz=0)
#cmds.connectDynamic("fourParticles", f="gravityField1")
#
#cmds.select("fourParticles", "curve1", "simpleSpring", r=True)
#
#cmds.spring(add=True, noDuplicate=False, minMax=True, mnd=0, mxd=0, useRestLengthPS=True, s=1, d=0.2, sfw=1, efw=1)
#
#cmds.playbackOptions(e=True, min=0.00, max=600.0)
#cmds.currentTime(0, e=True)
#cmds.play(wait=True, forward=True)
#


import sys

kPluginNodeTypeName = "spSimpleSpring"
spSimpleSpringNodeId = OpenMaya.MTypeId(0x80044)


class simpleSpring(OpenMayaMPx.MPxSpringNode):
	aSpringFactor = OpenMaya.MObject()

	def __init__(self):
		OpenMayaMPx.MPxSpringNode.__init__(self)
		self.__myFactor = 0.0


	def compute(self, plug, block):
		"""
		 In this simple example, do nothing in this method. But get the
		 spring factor here for "applySpringLaw" to compute output force.

		 Note: always let this method return "kUnknownParameter" so that 
		 "applySpringLaw" can be called when Maya needs to compute spring force.

		 It is recommended to only override compute() to get user defined
		 attributes.
		"""
		# Get spring factor,
		self.__myFactor = self.springFactor(block)

		# Note: return "kUnknownParameter" so that Maya spring node can
		# compute spring force for this plug-in simple spring node.
		return OpenMaya.kUnknownParameter


	def applySpringLaw(self, stiffness, damping, restLength, endMass1, endMass2,
						endP1, endP2, endV1, endV2, forceV1, forceV2):
		"""
		 In this overridden method, the attribute, aSpringFactor, is used
		 to compute output force with a simple spring law.

		    F = - factor * (L - restLength) * Vector of (endP1 - endP2).
		"""
		distV = endP1 - endP2
		L = distV.length()
		distV.normalize()

		F = self.__myFactor * (L - restLength)

		v1 = distV * -F
		forceV1.x = v1.x
		forceV1.y = v1.y
		forceV1.z = v1.z

		v2 = -forceV1
		forceV2.x = v2.x
		forceV2.y = v2.y
		forceV2.z = v2.z


	def springFactor(self, block):
		hValue = block.inputValue(simpleSpring.aSpringFactor)
		value = hValue.asDouble()
		return value


	def end1WeightValue(self, block):
		hValue = block.inputValue(simpleSpring.mEnd1Weight)
		value = hValue.asDouble()
		return value


	def end2WeightValue(self, block):
		hValue = block.inputValue(simpleSpring.mEnd2Weight)
		value = hValue.asDouble()
		return value


#####################################################################


def creator():
	return OpenMayaMPx.asMPxPtr(simpleSpring())


def initializer():
	numAttr = OpenMaya.MFnNumericAttribute()
	
	simpleSpring.aSpringFactor = numAttr.create("springFactor", "sf", OpenMaya.MFnNumericData.kDouble, 1.0)
	numAttr.setKeyable(True)
	simpleSpring.addAttribute(simpleSpring.aSpringFactor)


# initialize the script plug-in
def initializePlugin(mobject):
	mplugin = OpenMayaMPx.MFnPlugin(mobject, "Autodesk", "3.0", "Any")
	try:
		mplugin.registerNode(kPluginNodeTypeName, spSimpleSpringNodeId,
								creator, initializer, OpenMayaMPx.MPxNode.kSpringNode)
	except:
		sys.stderr.write( "Failed to register node: %s" % kPluginNodeTypeName )
		raise


# uninitialize the script plug-in
def uninitializePlugin(mobject):
	mplugin = OpenMayaMPx.MFnPlugin(mobject)
	try:
		mplugin.deregisterNode( spSimpleSpringNodeId )
	except:
		sys.stderr.write( "Failed to deregister node: %s" % kPluginNodeTypeName )
		raise
