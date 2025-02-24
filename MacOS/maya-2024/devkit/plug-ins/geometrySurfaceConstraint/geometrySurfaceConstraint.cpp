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

#include "geometrySurfaceConstraint.h"

////////////////////////////////////////////////////////////////////////
//
// DESCRIPTION:
//
// Produces the MPxConstraint node "geometrySurfaceConstraint" and MEL command "geometrySurfaceConstraint".

// This example demonstrates how to use the MPxConstraint and MPxConstraintCommand classes to create a 
// geometry constraint. This type of constraint will keep the constrained object attached to the target 
// as the target is moved. The constrained object can be constrained to one of multiple targets.
// You can choose to constrain to the target of the highest or lowest weight.
//
// To use this plug-in, first load it and then execute:
//
//	loadPlugin geometrySurfaceConstraint;
//
//	1. cylinder constrained to plane
//	file -f -new;
//	polyPlane -w 1 -h 1 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1;
//	scale -r 15 15 15;
//	polyCylinder -r 1 -h 2 -sx 20 -sy 1 -sz 1 -ax 0 1 0 -rcp 0 -cuv 3 -ch 1;
//	select -cl;
//	select -r pPlane1 pCylinder1;
//	geometrySurfaceConstraint -weight 1;
//
//	2. cylinder constrained to one of two planes
//	depending on plane weight
//	file -f -new;
//	polyPlane -w 1 -h 1 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1;
//	scale -r 10 10 10;
//	polyPlane -w 1 -h 1 -sx 10 -sy 10 -ax 0 1 0 -cuv 2 -ch 1;
//	scale -r 15 15 15;
//	polyCylinder -r 1 -h 2 -sx 20 -sy 1 -sz 1 -ax 0 1 0 -rcp 0 -cuv 3 -ch 1;
//	select -cl;
//	select -r pPlane1 pPlane2 pCylinder1;
//	geometryConstraint -weight 1.0;
//  change plane weight to move constrained object
//	geometryConstraint -e -w 10.0 pPlane2 pCylinder1;
//
////////////////////////////////////////////////////////////////////////

//
//	Node implementation
//

MTypeId     geometrySurfaceConstraint::id( 0x8103F );
MObject     geometrySurfaceConstraint::compoundTarget;        
MObject     geometrySurfaceConstraint::targetGeometry;       
MObject     geometrySurfaceConstraint::targetWeight;       
MObject     geometrySurfaceConstraint::constraintParentInverseMatrix;       
MObject     geometrySurfaceConstraint::constraintGeometry;       


geometrySurfaceConstraint::geometrySurfaceConstraint() 
{
	weightType = geometrySurfaceConstraintCommand::kLargestWeight;
}

geometrySurfaceConstraint::~geometrySurfaceConstraint() 
{
}

void geometrySurfaceConstraint::postConstructor()
{
}

MStatus geometrySurfaceConstraint::compute( const MPlug& plug, MDataBlock& block )
{	
	MStatus returnStatus;
 
	if ( plug == geometrySurfaceConstraint::constraintGeometry )
	{
		//
		block.inputValue(constraintParentInverseMatrix);
		//
		MArrayDataHandle targetArray = block.inputArrayValue( compoundTarget );
		unsigned int targetArrayCount = targetArray.elementCount();
		double weight,selectedWeight = 0;
		if ( weightType == geometrySurfaceConstraintCommand::kSmallestWeight )
			selectedWeight = FLT_MAX;
		MObject selectedMesh;
		unsigned int i;
		for ( i = 0; i < targetArrayCount; i++ )
		{
			MDataHandle targetElement = targetArray.inputValue();
			weight = targetElement.child(targetWeight).asDouble();
			if ( !equivalent(weight,0.0))
			{
				if ( weightType == geometrySurfaceConstraintCommand::kLargestWeight )
				{
					if ( weight > selectedWeight )
					{
						MObject mesh = targetElement.child(targetGeometry).asMesh();
						if ( !mesh.isNull() )
						{
							selectedMesh = mesh;
							selectedWeight =  weight;
						}
					}
				}
				else
				{
					if  ( weight < selectedWeight )
					{
						MObject mesh = targetElement.child(targetGeometry).asMesh();
						if ( !mesh.isNull() )
						{
							selectedMesh = mesh;
							selectedWeight =  weight;
						}
					}
				}
			}
			targetArray.next();
		}
		//
		if ( selectedMesh.isNull() )
		{
			block.setClean(plug);
		}
		else
		{
			// The transform node via the geometry attribute will take care of
			// updating the location of the constrained geometry.
			MDataHandle outputConstraintGeometryHandle = block.outputValue(constraintGeometry);
			outputConstraintGeometryHandle.setMObject(selectedMesh);
		}
	} 
	else 
	{
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

const MObject geometrySurfaceConstraint::weightAttribute() const
{
	return geometrySurfaceConstraint::targetWeight;
}

const MObject geometrySurfaceConstraint::targetAttribute() const
{
	return geometrySurfaceConstraint::compoundTarget;
}

void geometrySurfaceConstraint::getOutputAttributes(MObjectArray& attributeArray)
{
	attributeArray.clear();
	attributeArray.append( geometrySurfaceConstraint::constraintGeometry );
}

void* geometrySurfaceConstraint::creator()
{
	return new geometrySurfaceConstraint();
}

MStatus geometrySurfaceConstraint::initialize()
{
	MFnNumericAttribute nAttr;
	MStatus				status;

	// constraint attributes

	{	// Geometry: mesh, readable, not writable, delete on disconnect
		MFnTypedAttribute typedAttrNotWritable;
		geometrySurfaceConstraint::constraintGeometry =
			typedAttrNotWritable.create( "constraintGeometry", "cg", MFnData::kMesh, MObject::kNullObj, &status ); 	
		if (!status) { status.perror("typedAttrNotWritable.create:cgeom"); return status;}
		status = typedAttrNotWritable.setReadable(true);
		if (!status) { status.perror("typedAttrNotWritable.setReadable:cgeom"); return status;}
		status = typedAttrNotWritable.setWritable(false);
		if (!status) { status.perror("typedAttrNotWritable.setWritable:cgeom"); return status;}
		status = typedAttrNotWritable.setDisconnectBehavior(MFnAttribute::kDelete);
		if (!status) { status.perror("typedAttrNotWritable.setDisconnectBehavior:cgeom"); return status;}
	}
	{	// Parent inverse matrix: delete on disconnect
		MFnTypedAttribute typedAttr;
		geometrySurfaceConstraint::constraintParentInverseMatrix =
			typedAttr.create( "constraintPim", "ci", MFnData::kMatrix, MObject::kNullObj, &status ); 	
		if (!status) { status.perror("typedAttr.create:matrix"); return status;}
		status = typedAttr.setDisconnectBehavior(MFnAttribute::kDelete);
		if (!status) { status.perror("typedAttr.setDisconnectBehavior:cgeom"); return status;}

		// Target geometry: mesh, delete on disconnect
		geometrySurfaceConstraint::targetGeometry =
			typedAttr.create( "targetGeometry", "tg", MFnData::kMesh, MObject::kNullObj, &status ); 	
		if (!status) { status.perror("typedAttr.create:tgeom"); return status;}
		status = typedAttr.setDisconnectBehavior(MFnAttribute::kDelete);
		if (!status) { status.perror("typedAttr.setDisconnectBehavior:cgeom"); return status;}
	}
	{	// Target weight: double, min 0, default 1.0, keyable, delete on disconnect
		MFnNumericAttribute typedAttrKeyable;
		geometrySurfaceConstraint::targetWeight 
			= typedAttrKeyable.create( "weight", "wt", MFnNumericData::kDouble, 1.0, &status );
		if (!status) { status.perror("typedAttrKeyable.create:weight"); return status;}
		status = typedAttrKeyable.setMin( (double) 0 );
		if (!status) { status.perror("typedAttrKeyable.setMin"); return status;}
		status = typedAttrKeyable.setKeyable( true );
		if (!status) { status.perror("typedAttrKeyable.setKeyable"); return status;}
		status = typedAttrKeyable.setDisconnectBehavior(MFnAttribute::kDelete);
		if (!status) { status.perror("typedAttrKeyable.setDisconnectBehavior:cgeom"); return status;}
	}
	{	// Compound target(geometry,weight): array, delete on disconnect
		MFnCompoundAttribute compoundAttr;
		geometrySurfaceConstraint::compoundTarget = 
			compoundAttr.create( "target", "tgt",&status );
		if (!status) { status.perror("compoundAttr.create"); return status;}
		status = compoundAttr.addChild( geometrySurfaceConstraint::targetGeometry );
		if (!status) { status.perror("compoundAttr.addChild"); return status;}
		status = compoundAttr.addChild( geometrySurfaceConstraint::targetWeight );
		if (!status) { status.perror("compoundAttr.addChild"); return status;}
		status = compoundAttr.setArray( true );
		if (!status) { status.perror("compoundAttr.setArray"); return status;}
		status = compoundAttr.setDisconnectBehavior(MFnAttribute::kDelete);
		if (!status) { status.perror("typedAttrKeyable.setDisconnectBehavior:cgeom"); return status;}
	}

	status = addAttribute( geometrySurfaceConstraint::constraintParentInverseMatrix );
	if (!status) { status.perror("addAttribute"); return status;}
	status = addAttribute( geometrySurfaceConstraint::constraintGeometry );
	if (!status) { status.perror("addAttribute"); return status;}
	status = addAttribute( geometrySurfaceConstraint::compoundTarget );
	if (!status) { status.perror("addAttribute"); return status;}

	status = attributeAffects( compoundTarget, constraintGeometry );
	if (!status) { status.perror("attributeAffects"); return status;}
	status = attributeAffects( targetGeometry, constraintGeometry );
	if (!status) { status.perror("attributeAffects"); return status;}
	status = attributeAffects( targetWeight, constraintGeometry );
	if (!status) { status.perror("attributeAffects"); return status;}
	status = attributeAffects( constraintParentInverseMatrix, constraintGeometry );
	if (!status) { status.perror("attributeAffects"); return status;}

	return MS::kSuccess;
}

//
//	Command implementation
//

geometrySurfaceConstraintCommand::geometrySurfaceConstraintCommand() {}
geometrySurfaceConstraintCommand::~geometrySurfaceConstraintCommand() {}

void* geometrySurfaceConstraintCommand::creator()
{
	return new geometrySurfaceConstraintCommand();
}

void geometrySurfaceConstraintCommand::createdConstraint(MPxConstraint *constraint)
{
	if ( constraint )
	{
		geometrySurfaceConstraint *c = (geometrySurfaceConstraint*) constraint;
		c->weightType = weightType;
	}
	else
	{
		MGlobal::displayError("Failed to get created constraint.");
	}
}


MStatus geometrySurfaceConstraintCommand::parseArgs(const MArgList &argList)
{
	MStatus			ReturnStatus;
	MArgDatabase	argData(syntax(), argList, &ReturnStatus);

	if ( ReturnStatus.error() )
		return MS::kFailure;

	// Settings only work at creation time. Would need an
	// attribute on the node in order to push this state
	// into the node at any time.
	ConstraintType typ;
	if (argData.isFlagSet(kConstrainToLargestWeightFlag))
		typ = geometrySurfaceConstraintCommand::kLargestWeight;
	else if (argData.isFlagSet(kConstrainToSmallestWeightFlag))
		typ = geometrySurfaceConstraintCommand::kSmallestWeight;
	else
		typ = geometrySurfaceConstraintCommand::kLargestWeight;
	weightType = typ;

	// Need parent to process
	return MS::kUnknownParameter;
}

MStatus geometrySurfaceConstraintCommand::doIt(const MArgList &argList)
{
	MStatus ReturnStatus;

	if ( MS::kFailure == parseArgs(argList) )
		return MS::kFailure;

	return MS::kUnknownParameter;
}

MStatus geometrySurfaceConstraintCommand::connectTarget(MDagPath& opaqueTarget, int index)
{
	try
	{
		MObject targetObject = opaqueTarget.node();
		MFnDagNode targetDagNode(targetObject);
		MObject targetAttribute = targetDagNode.attribute("worldMesh");

		MStatus status = connectTargetAttribute(
			opaqueTarget,
			index,
			targetAttribute,
			geometrySurfaceConstraint::targetGeometry,
			false);

		if (!status)
		{
			status.perror("connectTargetGeometry");
			return status;
		}
	}
	catch (...)
	{
		return MS::kFailure;
	}

	return MS::kSuccess;
}

MStatus geometrySurfaceConstraintCommand::connectObjectAndConstraint( MDGModifier& modifier )
{
	MObject transform = transformObject();
	if ( transform.isNull() )
	{
		MGlobal::displayError("Failed to get transformObject()");
		return MS::kFailure;
	}

	MStatus status;
	MFnTransform transformFn( transform );
	MVector translate = transformFn.getTranslation(MSpace::kTransform,&status);
	if (!status) { status.perror(" transformFn.getTranslation"); return status;}

	MPlug translatePlug = transformFn.findPlug( "translate",  true,  &status );
	if (!status) { status.perror(" transformFn.findPlug"); return status;}

	if ( MPlug::kFreeToChange == translatePlug.isFreeToChange() )
	{
		MFnNumericData nd;
		MObject translateData = nd.create( MFnNumericData::k3Double, &status );
		status = nd.setData3Double( translate.x,translate.y,translate.z);
		if (!status) { status.perror("nd.setData3Double"); return status;}
		status = modifier.newPlugValue( translatePlug, translateData );
		if (!status) { status.perror("modifier.newPlugValue"); return status;}

		status = connectObjectAttribute( 
			MPxTransform::geometry, 
					geometrySurfaceConstraint::constraintGeometry, false );
		if (!status) { status.perror("connectObjectAttribute"); return status;}
	}

	status = connectObjectAttribute( 
		MPxTransform::parentInverseMatrix,
			geometrySurfaceConstraint::constraintParentInverseMatrix, true, true );
	if (!status) { status.perror("connectObjectAttribute"); return status;}

	return MS::kSuccess;
}

const MObject& geometrySurfaceConstraintCommand::constraintInstancedAttribute() const
{
	return geometrySurfaceConstraint::constraintParentInverseMatrix;
}

const MObject& geometrySurfaceConstraintCommand::constraintOutputAttribute() const
{
	return geometrySurfaceConstraint::constraintGeometry;
}

const MObject& geometrySurfaceConstraintCommand::constraintTargetInstancedAttribute() const
{
	return geometrySurfaceConstraint::targetGeometry;
}

const MObject& geometrySurfaceConstraintCommand::constraintTargetAttribute() const
{
	return geometrySurfaceConstraint::compoundTarget;
}

const MObject& geometrySurfaceConstraintCommand::constraintTargetWeightAttribute() const
{
	return geometrySurfaceConstraint::targetWeight;
}

const MObject& geometrySurfaceConstraintCommand::objectAttribute() const
{
	return MPxTransform::geometry;
}

MTypeId geometrySurfaceConstraintCommand::constraintTypeId() const
{
	return geometrySurfaceConstraint::id;
}

MPxConstraintCommand::TargetType geometrySurfaceConstraintCommand::targetType() const
{
	return MPxConstraintCommand::kGeometryShape;
}

MStatus geometrySurfaceConstraintCommand::appendSyntax()
{
	MStatus ReturnStatus;

	MSyntax theSyntax = syntax(&ReturnStatus);
	if (MS::kSuccess != ReturnStatus) {
		MGlobal::displayError("Could not get the parent's syntax");
		return ReturnStatus;
	}

	// Add our command flags
	theSyntax.addFlag( kConstrainToLargestWeightFlag, kConstrainToLargestWeightFlagLong );
	theSyntax.addFlag( kConstrainToSmallestWeightFlag, kConstrainToSmallestWeightFlagLong );

	return ReturnStatus;
}

//
//	Entry points
//

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "9.0", "Any");

	status = plugin.registerNode( "geometrySurfaceConstraint", geometrySurfaceConstraint::id, geometrySurfaceConstraint::creator,
		geometrySurfaceConstraint::initialize, MPxNode::kConstraintNode );
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	status = plugin.registerConstraintCommand( "geometrySurfaceConstraint", geometrySurfaceConstraintCommand::creator );
	if (!status) {
		status.perror("registerConstraintCommand");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( geometrySurfaceConstraint::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	status = plugin.deregisterConstraintCommand( "geometrySurfaceConstraint" );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
