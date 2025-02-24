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
// DISCLAIMER: THIS PLUGIN IS PROVIDED AS IS.  IT IS NOT SUPPORTED BY
//            AUTODESK, SO PLEASE USE AND MODIFY AT YOUR OWN RISK.
//
// PLUGIN NAME: closestPointOnCurve v1.0
// FILE: closestPointOnCurveNode.cpp
// DESCRIPTION: -Defines "closestPointOnCurve" node.
//              -Please see readme.txt for full details.
// AUTHOR: QT
// REFERENCES: -This plugin's concept is based off of the "closestPointOnSurface" node.
//             -The MEL script AEclosestPointOnSurfaceTemplate.mel was referred to for
//              the AE template MEL script that accompanies the closestPointOnCurve node.
// LAST UPDATED: Oct. 13th, 2001.
// COMPILED AND TESTED ON: Maya 4.0 on Windows

// HEADER FILES:
#include "closestPointOnCurveNode.h"
#include "closestTangentUAndDistance.h"

// DEFINE CLASS'S STATIC DATA MEMBERS:
MTypeId closestPointOnCurveNode::id(0x00105482);
MObject closestPointOnCurveNode::aInCurve;
MObject closestPointOnCurveNode::aInPosition;
MObject closestPointOnCurveNode::aInPositionX;
MObject closestPointOnCurveNode::aInPositionY;
MObject closestPointOnCurveNode::aInPositionZ;
MObject closestPointOnCurveNode::aPosition;
MObject closestPointOnCurveNode::aPositionX;
MObject closestPointOnCurveNode::aPositionY;
MObject closestPointOnCurveNode::aPositionZ;
MObject closestPointOnCurveNode::aNormal;
MObject closestPointOnCurveNode::aNormalX;
MObject closestPointOnCurveNode::aNormalY;
MObject closestPointOnCurveNode::aNormalZ;
MObject closestPointOnCurveNode::aTangent;
MObject closestPointOnCurveNode::aTangentX;
MObject closestPointOnCurveNode::aTangentY;
MObject closestPointOnCurveNode::aTangentZ;
MObject closestPointOnCurveNode::aParamU;
MObject closestPointOnCurveNode::aDistance;

// CONSTRUCTOR DEFINITION:
closestPointOnCurveNode::closestPointOnCurveNode()
{}

// DESTRUCTOR DEFINITION:
closestPointOnCurveNode::~closestPointOnCurveNode()
{}

// FOR CREATING AN INSTANCE OF THIS NODE:
void *closestPointOnCurveNode::creator()
{
   return new closestPointOnCurveNode();
}

// INITIALIZES THE NODE BY CREATING ITS ATTRIBUTES:
MStatus closestPointOnCurveNode::initialize()
{
   // CREATE AND ADD ".inCurve" ATTRIBUTE:
   MFnTypedAttribute inCurveAttrFn;
   aInCurve = inCurveAttrFn.create("inCurve", "ic", MFnData::kNurbsCurve);
   inCurveAttrFn.setStorable(true);
   inCurveAttrFn.setKeyable(false);
   inCurveAttrFn.setReadable(true);
   inCurveAttrFn.setWritable(true);
   inCurveAttrFn.setCached(false);
   addAttribute(aInCurve);

   // CREATE AND ADD ".inPositionX" ATTRIBUTE:
   MFnNumericAttribute inPositionXAttrFn;
   aInPositionX = inPositionXAttrFn.create("inPositionX", "ipx", MFnNumericData::kDouble, 0.0);
   inPositionXAttrFn.setStorable(true);
   inPositionXAttrFn.setKeyable(true);
   inPositionXAttrFn.setReadable(true);
   inPositionXAttrFn.setWritable(true);
   addAttribute(aInPositionX);

   // CREATE AND ADD ".inPositionY" ATTRIBUTE:
   MFnNumericAttribute inPositionYAttrFn;
   aInPositionY = inPositionYAttrFn.create("inPositionY", "ipy", MFnNumericData::kDouble, 0.0);
   inPositionYAttrFn.setStorable(true);
   inPositionYAttrFn.setKeyable(true);
   inPositionYAttrFn.setReadable(true);
   inPositionYAttrFn.setWritable(true);
   addAttribute(aInPositionY);

   // CREATE AND ADD ".inPositionZ" ATTRIBUTE:
   MFnNumericAttribute inPositionZAttrFn;
   aInPositionZ = inPositionZAttrFn.create("inPositionZ", "ipz", MFnNumericData::kDouble, 0.0);
   inPositionZAttrFn.setStorable(true);
   inPositionZAttrFn.setKeyable(true);
   inPositionZAttrFn.setReadable(true);
   inPositionZAttrFn.setWritable(true);
   addAttribute(aInPositionZ);

   // CREATE AND ADD ".inPosition" ATTRIBUTE:
   MFnNumericAttribute inPositionAttrFn;
   aInPosition = inPositionAttrFn.create("inPosition", "ip", aInPositionX, aInPositionY, aInPositionZ);
   inPositionAttrFn.setStorable(true);
   inPositionAttrFn.setKeyable(true);
   inPositionAttrFn.setReadable(true);
   inPositionAttrFn.setWritable(true);
   addAttribute(aInPosition);

   // CREATE AND ADD ".positionX" ATTRIBUTE:
   MFnNumericAttribute pointXAttrFn;
   aPositionX = pointXAttrFn.create("positionX", "px", MFnNumericData::kDouble, 0.0);
   pointXAttrFn.setStorable(false);
   pointXAttrFn.setKeyable(false);
   pointXAttrFn.setReadable(true);
   pointXAttrFn.setWritable(false);
   addAttribute(aPositionX);

   // CREATE AND ADD ".positionY" ATTRIBUTE:
   MFnNumericAttribute pointYAttrFn;
   aPositionY = pointYAttrFn.create("positionY", "py", MFnNumericData::kDouble, 0.0);
   pointYAttrFn.setStorable(false);
   pointYAttrFn.setKeyable(false);
   pointYAttrFn.setReadable(true);
   pointYAttrFn.setWritable(false);
   addAttribute(aPositionY);

   // CREATE AND ADD ".positionZ" ATTRIBUTE:
   MFnNumericAttribute pointZAttrFn;
   aPositionZ = pointZAttrFn.create("positionZ", "pz", MFnNumericData::kDouble, 0.0);
   pointZAttrFn.setStorable(false);
   pointZAttrFn.setKeyable(false);
   pointZAttrFn.setReadable(true);
   pointZAttrFn.setWritable(false);
   addAttribute(aPositionZ);

   // CREATE AND ADD ".position" ATTRIBUTE:
   MFnNumericAttribute pointAttrFn;
   aPosition = pointAttrFn.create("position", "p", aPositionX, aPositionY, aPositionZ);
   pointAttrFn.setStorable(false);
   pointAttrFn.setKeyable(false);
   pointAttrFn.setReadable(true);
   pointAttrFn.setWritable(false);
   addAttribute(aPosition);

   // CREATE AND ADD ".normalX" ATTRIBUTE:
   MFnNumericAttribute normalXAttrFn;
   aNormalX = normalXAttrFn.create("normalX", "nx", MFnNumericData::kDouble, 0.0);
   normalXAttrFn.setStorable(false);
   normalXAttrFn.setKeyable(false);
   normalXAttrFn.setReadable(true);
   normalXAttrFn.setWritable(false);
   addAttribute(aNormalX);

   // CREATE AND ADD ".normalY" ATTRIBUTE:
   MFnNumericAttribute normalYAttrFn;
   aNormalY = normalYAttrFn.create("normalY", "ny", MFnNumericData::kDouble, 0.0);
   normalYAttrFn.setStorable(false);
   normalYAttrFn.setKeyable(false);
   normalYAttrFn.setReadable(true);
   normalYAttrFn.setWritable(false);
   addAttribute(aNormalY);

   // CREATE AND ADD ".normalZ" ATTRIBUTE:
   MFnNumericAttribute normalZAttrFn;
   aNormalZ = normalZAttrFn.create("normalZ", "nz", MFnNumericData::kDouble, 0.0);
   normalZAttrFn.setStorable(false);
   normalZAttrFn.setKeyable(false);
   normalZAttrFn.setReadable(true);
   normalZAttrFn.setWritable(false);
   addAttribute(aNormalZ);

   // CREATE AND ADD ".normal" ATTRIBUTE:
   MFnNumericAttribute normalAttrFn;
   aNormal = normalAttrFn.create("normal", "n", aNormalX, aNormalY, aNormalZ);
   normalAttrFn.setStorable(false);
   normalAttrFn.setKeyable(false);
   normalAttrFn.setReadable(true);
   normalAttrFn.setWritable(false);
   addAttribute(aNormal);

   // CREATE AND ADD ".tangentX" ATTRIBUTE:
   MFnNumericAttribute tangentXAttrFn;
   aTangentX = tangentXAttrFn.create("tangentX", "tx", MFnNumericData::kDouble, 0.0);
   tangentXAttrFn.setStorable(false);
   tangentXAttrFn.setKeyable(false);
   tangentXAttrFn.setReadable(true);
   tangentXAttrFn.setWritable(false);
   addAttribute(aTangentX);

   // CREATE AND ADD ".tangentY" ATTRIBUTE:
   MFnNumericAttribute tangentYAttrFn;
   aTangentY = tangentYAttrFn.create("tangentY", "ty", MFnNumericData::kDouble, 0.0);
   tangentYAttrFn.setStorable(false);
   tangentYAttrFn.setKeyable(false);
   tangentYAttrFn.setReadable(true);
   tangentYAttrFn.setWritable(false);
   addAttribute(aTangentY);

   // CREATE AND ADD ".tangentZ" ATTRIBUTE:
   MFnNumericAttribute tangentZAttrFn;
   aTangentZ = tangentZAttrFn.create("tangentZ", "tz", MFnNumericData::kDouble, 0.0);
   tangentZAttrFn.setStorable(false);
   tangentZAttrFn.setKeyable(false);
   tangentZAttrFn.setReadable(true);
   tangentZAttrFn.setWritable(false);
   addAttribute(aTangentZ);

   // CREATE AND ADD ".tangent" ATTRIBUTE:

   MFnNumericAttribute tangentAttrFn;
   aTangent = tangentAttrFn.create("tangent", "t", aTangentX, aTangentY, aTangentZ);
   tangentAttrFn.setStorable(false);
   tangentAttrFn.setKeyable(false);
   tangentAttrFn.setReadable(true);
   tangentAttrFn.setWritable(false);
   addAttribute(aTangent);

   // CREATE AND ADD ".parameU" ATTRIBUTE:
   MFnNumericAttribute paramUAttrFn;
   aParamU = paramUAttrFn.create("paramU", "u", MFnNumericData::kDouble, 0.0);
   paramUAttrFn.setStorable(false);
   paramUAttrFn.setKeyable(false);
   paramUAttrFn.setReadable(true);
   paramUAttrFn.setWritable(false);
   addAttribute(aParamU);

   // CREATE AND ADD ".distance" ATTRIBUTE:
   MFnNumericAttribute distanceAttrFn;
   aDistance = distanceAttrFn.create("distance", "d", MFnNumericData::kDouble, 0.0);
   distanceAttrFn.setStorable(false);
   distanceAttrFn.setKeyable(false);
   distanceAttrFn.setReadable(true);
   distanceAttrFn.setWritable(false);
   addAttribute(aDistance);

   // DEPENDENCY RELATIONS FOR ".inCurve":
   attributeAffects(aInCurve, aPosition);
   attributeAffects(aInCurve, aPositionX);
   attributeAffects(aInCurve, aPositionY);
   attributeAffects(aInCurve, aPositionZ);
   attributeAffects(aInCurve, aNormal);
   attributeAffects(aInCurve, aNormalX);
   attributeAffects(aInCurve, aNormalY);
   attributeAffects(aInCurve, aNormalZ);
   attributeAffects(aInCurve, aTangent);
   attributeAffects(aInCurve, aTangentX);
   attributeAffects(aInCurve, aTangentY);
   attributeAffects(aInCurve, aTangentZ);
   attributeAffects(aInCurve, aParamU);
   attributeAffects(aInCurve, aDistance);

   // DEPENDENCY RELATIONS FOR ".inPosition":
   attributeAffects(aInPosition, aPosition);
   attributeAffects(aInPosition, aPositionX);
   attributeAffects(aInPosition, aPositionY);
   attributeAffects(aInPosition, aPositionZ);
   attributeAffects(aInPosition, aNormal);
   attributeAffects(aInPosition, aNormalX);
   attributeAffects(aInPosition, aNormalY);
   attributeAffects(aInPosition, aNormalZ);
   attributeAffects(aInPosition, aTangent);
   attributeAffects(aInPosition, aTangentX);
   attributeAffects(aInPosition, aTangentY);
   attributeAffects(aInPosition, aTangentZ);
   attributeAffects(aInPosition, aParamU);
   attributeAffects(aInPosition, aDistance);

   // DEPENDENCY RELATIONS FOR ".inPositionX":
   attributeAffects(aInPositionX, aPosition);
   attributeAffects(aInPositionX, aPositionX);
   attributeAffects(aInPositionX, aPositionY);
   attributeAffects(aInPositionX, aPositionZ);
   attributeAffects(aInPositionX, aNormal);
   attributeAffects(aInPositionX, aNormalX);
   attributeAffects(aInPositionX, aNormalY);
   attributeAffects(aInPositionX, aNormalZ);
   attributeAffects(aInPositionX, aTangent);
   attributeAffects(aInPositionX, aTangentX);
   attributeAffects(aInPositionX, aTangentY);
   attributeAffects(aInPositionX, aTangentZ);
   attributeAffects(aInPositionX, aParamU);
   attributeAffects(aInPositionX, aDistance);

   // DEPENDENCY RELATIONS FOR ".inPositionY":
   attributeAffects(aInPositionY, aPosition);
   attributeAffects(aInPositionY, aPositionX);
   attributeAffects(aInPositionY, aPositionY);
   attributeAffects(aInPositionY, aPositionZ);
   attributeAffects(aInPositionY, aNormal);
   attributeAffects(aInPositionY, aNormalX);
   attributeAffects(aInPositionY, aNormalY);
   attributeAffects(aInPositionY, aNormalZ);
   attributeAffects(aInPositionY, aTangent);
   attributeAffects(aInPositionY, aTangentX);
   attributeAffects(aInPositionY, aTangentY);
   attributeAffects(aInPositionY, aTangentZ);
   attributeAffects(aInPositionY, aParamU);
   attributeAffects(aInPositionY, aDistance);

   // DEPENDENCY RELATIONS FOR ".inPositionZ":
   attributeAffects(aInPositionZ, aPosition);
   attributeAffects(aInPositionZ, aPositionX);
   attributeAffects(aInPositionZ, aPositionY);
   attributeAffects(aInPositionZ, aPositionZ);
   attributeAffects(aInPositionZ, aNormal);
   attributeAffects(aInPositionZ, aNormalX);
   attributeAffects(aInPositionZ, aNormalY);
   attributeAffects(aInPositionZ, aNormalZ);
   attributeAffects(aInPositionZ, aTangent);
   attributeAffects(aInPositionZ, aTangentX);
   attributeAffects(aInPositionZ, aTangentY);
   attributeAffects(aInPositionZ, aTangentZ);
   attributeAffects(aInPositionZ, aParamU);
   attributeAffects(aInPositionZ, aDistance);

   return MS::kSuccess;
}





// COMPUTE METHOD'S DEFINITION:

MStatus closestPointOnCurveNode::compute(const MPlug &plug, MDataBlock &data)

{
   // DO THE COMPUTE ONLY FOR THE *OUTPUT* PLUGS THAT ARE DIRTIED:
   if ((plug == aPosition)  || (plug == aPositionX)  || (plug == aPositionY)  || (plug == aPositionZ)
    || (plug == aNormal) || (plug == aNormalX) || (plug == aNormalY) || (plug == aNormalZ)
    || (plug == aTangent) || (plug == aTangentX) || (plug == aTangentY) || (plug == aTangentZ)
    || (plug == aParamU) || (plug == aDistance))
   {
      // READ IN ".inCurve" DATA:
      MDataHandle inCurveDataHandle = data.inputValue(aInCurve);
      MObject inCurve = inCurveDataHandle.asNurbsCurve();

      // READ IN ".inPositionX" DATA:
      MDataHandle inPositionXDataHandle = data.inputValue(aInPositionX);
      double inPositionX = inPositionXDataHandle.asDouble();

      // READ IN ".inPositionY" DATA:
      MDataHandle inPositionYDataHandle = data.inputValue(aInPositionY);
      double inPositionY = inPositionYDataHandle.asDouble();

      // READ IN ".inPositionZ" DATA:
      MDataHandle inPositionZDataHandle = data.inputValue(aInPositionZ);
      double inPositionZ = inPositionZDataHandle.asDouble();

      // GET THE CLOSEST POSITION, NORMAL, TANGENT, PARAMETER-U AND DISTANCE:
      MPoint inPosition(inPositionX, inPositionY, inPositionZ), position;
      MVector normal, tangent;
      double paramU, distance;
      MDagPath dummyDagPath;

      closestTangentUAndDistance(dummyDagPath, inPosition, position, normal, tangent, paramU, distance, inCurve);

      // WRITE OUT ".position" DATA:
      MDataHandle positionDataHandle = data.outputValue(aPosition);
      positionDataHandle.set(position.x, position.y, position.z);
      data.setClean(plug);

      // WRITE OUT ".normal" DATA:
      MDataHandle normalDataHandle = data.outputValue(aNormal);
      normalDataHandle.set(normal.x, normal.y, normal.z);
      data.setClean(plug);

      // WRITE OUT ".tangent" DATA:
      MDataHandle tangentDataHandle = data.outputValue(aTangent);
      tangentDataHandle.set(tangent.x, tangent.y, tangent.z);
      data.setClean(plug);

      // WRITE OUT ".paramU" DATA:
      MDataHandle paramUDataHandle = data.outputValue(aParamU);
      paramUDataHandle.set(paramU);
      data.setClean(plug);

      // WRITE OUT ".distance" DATA:
      MDataHandle distanceDataHandle = data.outputValue(aDistance);
      distanceDataHandle.set(distance);
      data.setClean(plug);
   } else {
      return MS::kUnknownParameter;
   }

   return MS::kSuccess;
}

