#include <math.h>
#include <maya/MPxNode.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFnPlugin.h>

///////////////////////////////////////////////////////////////////
// DESCRIPTION: 
// Produces dependency graph node lightNode

// This node is an example of a directional light.

// The inputs for this node are direction, color, and some boolean flags to indicate if the light is contributing to 
// ambient, diffuse, and specular components. There is also a position input that can receive a connection from a 3d 
// manipulator for placement within the scene.

// The output attribute of the LightNode node is a compound attribute called "lightData". To use this shader, 
// create a LightNode and modify its inputs to see the different illumination results. 
//
///////////////////////////////////////////////////////////////////

class LightNode : public MPxNode
{
	public:
                      LightNode();
              ~LightNode() override;

    MStatus   compute( const MPlug&, MDataBlock& ) override;
    SchedulingType schedulingType() const override { return SchedulingType::kParallel; }

    static void *     creator();
    static MStatus    initialize();

    static MTypeId    id;

	private
:
	// Inputs
	static MObject  aColor;
	static MObject  aPosition;
	static MObject  aInputDirection;
    static MObject  aInputAmbient;
    static MObject  aInputDiffuse;
    static MObject  aInputSpecular;
    static MObject  aIntensity;

// Outputs
	static MObject  aLightDirection;
	static MObject  aLightIntensity;
    static MObject  aLightAmbient;
    static MObject  aLightDiffuse;
    static MObject  aLightSpecular;
    static MObject  aLightShadowFraction;
    static MObject  aPreShadowIntensity;
    static MObject  aLightBlindData;
    static MObject  aLightData;
};

MTypeId LightNode::id( 0x81010 );

MObject  LightNode::aColor;
MObject  LightNode::aPosition;
MObject  LightNode::aInputDirection;
MObject  LightNode::aInputAmbient;
MObject  LightNode::aInputDiffuse;
MObject  LightNode::aInputSpecular;
MObject  LightNode::aIntensity; 

MObject  LightNode::aLightData;
MObject  LightNode::aLightDirection;
MObject  LightNode::aLightIntensity;
MObject  LightNode::aLightAmbient;
MObject  LightNode::aLightDiffuse;
MObject  LightNode::aLightSpecular;
MObject  LightNode::aLightShadowFraction;
MObject  LightNode::aPreShadowIntensity;
MObject  LightNode::aLightBlindData;

//
// DESCRIPTION:
///////////////////////////////////////////////////////
LightNode::LightNode()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
LightNode::~LightNode()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void * LightNode::creator()
{
    return new LightNode();
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus LightNode::initialize()
{
    MFnNumericAttribute nAttr; 
    MFnLightDataAttribute lAttr;

    aColor = nAttr.createColor( "color", "c" );
    CHECK_MSTATUS ( nAttr.setKeyable(true) );
    CHECK_MSTATUS ( nAttr.setStorable(true) );
    CHECK_MSTATUS ( nAttr.setDefault(0.0f, 0.58824f, 0.644f) );

    aPosition = nAttr.createPoint( "position", "pos" );
    CHECK_MSTATUS ( nAttr.setKeyable(true) );
    CHECK_MSTATUS ( nAttr.setStorable(true) );

    aInputDirection = nAttr.createPoint( "inputDirection", "id" );
    CHECK_MSTATUS ( nAttr.setKeyable(true) );
    CHECK_MSTATUS ( nAttr.setStorable(true) );
    CHECK_MSTATUS ( nAttr.setDefault(-1.0f, 0.0f, 0.0f) );

    aInputAmbient = nAttr.create( "ambientOn", "an", MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setKeyable(true) );
    CHECK_MSTATUS ( nAttr.setStorable(true) );
    CHECK_MSTATUS ( nAttr.setHidden(false) );
    CHECK_MSTATUS ( nAttr.setDefault(true) );

    aInputDiffuse = nAttr.create( "diffuseOn", "dn", MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setKeyable(true) );
    CHECK_MSTATUS ( nAttr.setStorable(true) );
    CHECK_MSTATUS ( nAttr.setHidden(false) );
    CHECK_MSTATUS ( nAttr.setDefault(true) );

    aInputSpecular = nAttr.create( "specularOn", "sn", MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setKeyable(true) );
    CHECK_MSTATUS ( nAttr.setStorable(true) );
    CHECK_MSTATUS ( nAttr.setHidden(false) );
    CHECK_MSTATUS ( nAttr.setDefault(true) );

    aIntensity = nAttr.create( "intensity", "i", MFnNumericData::kFloat);
    CHECK_MSTATUS ( nAttr.setKeyable(true) );
    CHECK_MSTATUS ( nAttr.setStorable(true) );
    CHECK_MSTATUS ( nAttr.setHidden(false) );
    CHECK_MSTATUS ( nAttr.setDefault(1.0f) );

// Outputs

    aLightDirection = nAttr.createPoint( "lightDirection", "ld" );
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    CHECK_MSTATUS ( nAttr.setDefault(-1.0f, 0.0f, 0.0f) );

    aLightIntensity = nAttr.createColor( "lightIntensity", "li" );
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    CHECK_MSTATUS ( nAttr.setDefault(1.0f, 0.5f, 0.2f) );

    aLightAmbient = nAttr.create( "lightAmbient", "la", 
								  MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    nAttr.setDefault(true);

    aLightDiffuse = nAttr.create( "lightDiffuse", "ldf",
								  MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    CHECK_MSTATUS ( nAttr.setDefault(true) );

    aLightSpecular = nAttr.create( "lightSpecular", "ls", 
								   MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    CHECK_MSTATUS ( nAttr.setDefault(true) );

    aLightShadowFraction = nAttr.create("lightShadowFraction","lsf",
										MFnNumericData::kFloat);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    CHECK_MSTATUS ( nAttr.setDefault(0.0f) );

    aPreShadowIntensity = nAttr.create("preShadowIntensity","psi",
									   MFnNumericData::kFloat);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    CHECK_MSTATUS ( nAttr.setDefault(0.0f) );

    aLightBlindData = nAttr.createAddr("lightBlindData","lbld");
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightData = lAttr.create( "lightData", "ltd", 
                               aLightDirection, aLightIntensity, 
							   aLightAmbient, 
                               aLightDiffuse, aLightSpecular, 
							   aLightShadowFraction,
                               aPreShadowIntensity, aLightBlindData);
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );
    CHECK_MSTATUS ( lAttr.setStorable(false) );
    CHECK_MSTATUS ( lAttr.setHidden(true) );
    lAttr.setDefault(-1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.2f, true, true,
					 true, 0.0f, 1.0f, NULL);


    CHECK_MSTATUS ( addAttribute(aColor) );
    CHECK_MSTATUS ( addAttribute(aPosition) );
    CHECK_MSTATUS ( addAttribute(aInputDirection) );
    CHECK_MSTATUS ( addAttribute(aInputAmbient) );
    CHECK_MSTATUS ( addAttribute(aInputDiffuse) );
    CHECK_MSTATUS ( addAttribute(aInputSpecular) );
    CHECK_MSTATUS ( addAttribute(aIntensity) );
	
    CHECK_MSTATUS ( addAttribute(aLightData) );

    CHECK_MSTATUS ( attributeAffects (aLightIntensity, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aLightDirection, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aLightAmbient, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aLightDiffuse, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aLightSpecular, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aLightShadowFraction, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aPreShadowIntensity, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aLightBlindData, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aLightData, aLightData) );

    CHECK_MSTATUS ( attributeAffects (aColor, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aPosition, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aInputDirection, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aInputAmbient, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aInputDiffuse, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aInputSpecular, aLightData) );
    CHECK_MSTATUS ( attributeAffects (aIntensity, aLightData) );

    return MS::kSuccess;
}


//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus LightNode::compute(
const MPlug&      plug,
      MDataBlock& block ) 
{ 
    if ((plug != aLightData) && (plug.parent() != aLightData))
		return MS::kUnknownParameter;

    MFloatVector resultColor;

	// Real user input
    MFloatVector& LColor  = block.inputValue( aColor ).asFloatVector();
    // MFloatVector& Position  = block.inputValue( aPosition ).asFloatVector();
    float LIntensity  = block.inputValue( aIntensity ).asFloat();

	// Components to build LightData
    MFloatVector& LDirection  = block.inputValue( aInputDirection ).asFloatVector();
    bool  LAmbient  = block.inputValue( aInputAmbient ).asBool();
    bool  LDiffuse  = block.inputValue( aInputDiffuse ).asBool();
    bool  LSpecular = block.inputValue( aInputSpecular ).asBool();

// float LShadowF = block.inputValue( aLightShadowFraction ).asFloat();

    resultColor = LColor * LIntensity;
  
    // set ouput color attribute
    MDataHandle outLightDataHandle = block.outputValue( aLightData );

    MFloatVector& outIntensity = outLightDataHandle.child(aLightIntensity).asFloatVector();
    outIntensity = resultColor;

    MFloatVector& outDirection = outLightDataHandle.child(aLightDirection).asFloatVector();

    outDirection = LDirection;

    bool& outAmbient = outLightDataHandle.child(aLightAmbient).asBool();
    outAmbient = LAmbient;
    bool& outDiffuse = outLightDataHandle.child(aLightDiffuse).asBool();
    outDiffuse = LDiffuse;
    bool& outSpecular = outLightDataHandle.child(aLightSpecular).asBool();
    outSpecular = LSpecular;

    float& outSFraction = outLightDataHandle.child(aLightShadowFraction).asFloat();
    outSFraction = 1.0f;

    float& outPSIntensity = outLightDataHandle.child(aPreShadowIntensity).asFloat();
    outPSIntensity = (resultColor[0] + resultColor[1] + resultColor[2]) / 3.0f;

	void*& outBlindData = outLightDataHandle.child(aLightBlindData).asAddr();
	outBlindData = NULL;

    outLightDataHandle.setClean();


    return MS::kSuccess;
}


//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{ 
   const MString UserClassify( "light" );

   MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.5", "Any");
   CHECK_MSTATUS ( plugin.registerNode( "directLight", LightNode::id, 
                         LightNode::creator, LightNode::initialize,
                         MPxNode::kDependNode, &UserClassify ) );

   return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus uninitializePlugin( MObject obj )
{
   MFnPlugin plugin( obj );
   CHECK_MSTATUS ( plugin.deregisterNode( LightNode::id ) );

   return MS::kSuccess;
}
// =====================================================================
// Copyright 2018 Autodesk, Inc.  All rights reserved.
//
// This computer source code  and related  instructions and comments are
// the unpublished confidential and proprietary information of Autodesk,
// Inc. and are  protected  under applicable  copyright and trade secret
// law. They may not  be disclosed to, copied or used by any third party
// without the prior written consent of Autodesk, Inc.
// =====================================================================
