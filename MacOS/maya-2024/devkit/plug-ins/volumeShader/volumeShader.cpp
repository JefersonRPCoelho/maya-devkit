#include <math.h>
#include <maya/MPxNode.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFnPlugin.h>


///////////////////////////////////////////////////////
//
// DESCRIPTION:
// Produces dependency graph node VolumeNode
// 
// This node is an example of a volume shader. Volume shaders are used to apply color to specialized shapes 
// associated with light sources called "light shapes". One such set of shapes is created by the Light Fog effect. 
// The Light Fog effect can be assigned to any point or spot light through their respective Attribute Editors.
// 
// The output attribute of the VolumeNode node is called "outColor". To use this shader, create a VolumeNode node and connect
// its output to the "volumeShader" input of a Shading Group. The shading group must be connected to a light shape. 
//
///////////////////////////////////////////////////////

class VolumeNode : public MPxNode
{
	public:
	                 VolumeNode();
	        ~VolumeNode() override;

	MStatus compute( const MPlug&, MDataBlock& ) override;
    SchedulingType schedulingType() const override { return SchedulingType::kParallel; }

	static  void *  creator();
	static  MStatus initialize();

	static  MTypeId id;

	private:

	static MObject aColor;
	static MObject aInputValue;
	static MObject aOutColor;
	static MObject aOutTransparency;
	static MObject aFarPointC;
	static MObject aFarPointO;
	static MObject aFarPointW;
	static MObject aPointC;
	static MObject aPointO;
	static MObject aPointW;
	static MObject aToggleCamera;
	static MObject aToggleObject;
	static MObject aToggleWorld;
	static MObject aOutAlpha;
};


MTypeId VolumeNode::id( 0x81012 );
 
MObject VolumeNode::aColor ;
MObject VolumeNode::aInputValue;
MObject VolumeNode::aOutColor;
MObject VolumeNode::aOutTransparency;
MObject VolumeNode::aOutAlpha;
MObject VolumeNode::aFarPointC;
MObject VolumeNode::aFarPointO;
MObject VolumeNode::aFarPointW;
MObject VolumeNode::aPointC;
MObject VolumeNode::aPointO;
MObject VolumeNode::aPointW;
MObject VolumeNode::aToggleCamera;
MObject VolumeNode::aToggleObject;
MObject VolumeNode::aToggleWorld;

//
// DESCRIPTION:
///////////////////////////////////////////////////////
VolumeNode::VolumeNode()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
VolumeNode::~VolumeNode()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void* VolumeNode::creator()
{
    return new VolumeNode();
}



//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus VolumeNode::initialize()
{
	MFnNumericAttribute nAttr; 

	// Inputs
	aColor = nAttr.createColor( "color", "c" );
    CHECK_MSTATUS( nAttr.setKeyable( true ) );
    CHECK_MSTATUS( nAttr.setStorable( true ) );
    CHECK_MSTATUS( nAttr.setHidden(false) );
    CHECK_MSTATUS( nAttr.setDefault(1.0f, 1.0f, 1.0f) );

    aInputValue = nAttr.create( "distance", "d", MFnNumericData::kFloat);
    CHECK_MSTATUS( nAttr.setMin(0.0f) );
    CHECK_MSTATUS(nAttr.setMax(100000.0f) );
    CHECK_MSTATUS( nAttr.setSoftMax(1000.0f) );
    CHECK_MSTATUS( nAttr.setSoftMax(1000.0f) );
    CHECK_MSTATUS( nAttr.setKeyable(true) );
    CHECK_MSTATUS(nAttr.setStorable(true)  );
    CHECK_MSTATUS( nAttr.setDefault(1.0f) );


    aToggleCamera = nAttr.create( "cameraSpace", "cs", 
								  MFnNumericData::kBoolean);

    CHECK_MSTATUS( nAttr.setKeyable(true) );
    CHECK_MSTATUS( nAttr.setStorable(true) );
    CHECK_MSTATUS( nAttr.setHidden(false) );
    CHECK_MSTATUS( nAttr.setDefault(false) );

    aToggleObject = nAttr.create( "objectSpace", "os", 
								  MFnNumericData::kBoolean);
    CHECK_MSTATUS( nAttr.setKeyable(true) );
    CHECK_MSTATUS( nAttr.setStorable(true) );
    CHECK_MSTATUS( nAttr.setHidden(false) );
    CHECK_MSTATUS( nAttr.setDefault(false) );

    aToggleWorld = nAttr.create( "worldSpace", "ws", MFnNumericData::kBoolean);
    CHECK_MSTATUS(nAttr.setKeyable(true)  );
    CHECK_MSTATUS( nAttr.setStorable(true) );
    CHECK_MSTATUS(nAttr.setHidden(false)  );
    CHECK_MSTATUS( nAttr.setDefault(true) );

    aFarPointC = nAttr.createPoint("farPointCamera", "fc" );
    CHECK_MSTATUS(nAttr.setStorable(false)  );
    CHECK_MSTATUS( nAttr.setHidden(true) );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(true) );

    aFarPointO = nAttr.createPoint("farPointObj", "fo" );
    CHECK_MSTATUS( nAttr.setStorable(false) );
    CHECK_MSTATUS(nAttr.setHidden(true)  );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(true) );

    aFarPointW = nAttr.createPoint("farPointWorld", "fw" );
    CHECK_MSTATUS( nAttr.setStorable(false) );
    CHECK_MSTATUS( nAttr.setHidden(true) );
    CHECK_MSTATUS(nAttr.setReadable(true)  );
    CHECK_MSTATUS( nAttr.setWritable(true) );

    aPointC = nAttr.createPoint("pointCamera", "p" );
    CHECK_MSTATUS( nAttr.setStorable(false) );
    CHECK_MSTATUS( nAttr.setHidden(true) );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(true) );

    aPointO = nAttr.createPoint("pointObj", "po" );
    CHECK_MSTATUS( nAttr.setStorable(false)  );
    CHECK_MSTATUS( nAttr.setHidden(true) );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(true) );

    aPointW = nAttr.createPoint("pointWorld", "pw" );
    CHECK_MSTATUS( nAttr.setStorable(false) );
    CHECK_MSTATUS( nAttr.setHidden(true) );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(true) );

	// Outputs

    aOutColor = nAttr.createColor( "outColor", "oc" );
    CHECK_MSTATUS( nAttr.setStorable(false) );
    CHECK_MSTATUS( nAttr.setHidden(false) );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(false) );

    aOutTransparency = nAttr.createColor( "outTransparency", "ot" );
    CHECK_MSTATUS( nAttr.setStorable(false) );
    CHECK_MSTATUS( nAttr.setHidden(false) );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(false) );

    aOutAlpha = nAttr.create( "outAlpha", "oa", MFnNumericData::kFloat);
    CHECK_MSTATUS( nAttr.setStorable(false) );
    CHECK_MSTATUS( nAttr.setHidden(false) );
    CHECK_MSTATUS( nAttr.setReadable(true) );
    CHECK_MSTATUS( nAttr.setWritable(false) );

    CHECK_MSTATUS( addAttribute(aColor) );
    CHECK_MSTATUS( addAttribute(aInputValue) );
    CHECK_MSTATUS( addAttribute(aFarPointC) );
    CHECK_MSTATUS( addAttribute(aFarPointO) );
    CHECK_MSTATUS( addAttribute(aFarPointW) );
    CHECK_MSTATUS( addAttribute(aPointC) );
    CHECK_MSTATUS( addAttribute(aPointO) );
    CHECK_MSTATUS( addAttribute(aPointW) );
    CHECK_MSTATUS( addAttribute(aToggleCamera) );
    CHECK_MSTATUS( addAttribute(aToggleObject) );
    CHECK_MSTATUS( addAttribute(aToggleWorld) );

    CHECK_MSTATUS( addAttribute(aOutColor) );
    CHECK_MSTATUS( addAttribute(aOutTransparency) );
    CHECK_MSTATUS( addAttribute(aOutAlpha) );

    CHECK_MSTATUS( attributeAffects(aColor,  aOutColor) );
    CHECK_MSTATUS( attributeAffects(aColor,  aOutTransparency) );

    CHECK_MSTATUS( attributeAffects(aFarPointC, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aFarPointO, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aFarPointW, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aPointC, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aPointO, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aPointW, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aToggleCamera, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aToggleObject, aOutColor) );
    CHECK_MSTATUS( attributeAffects(aToggleWorld, aOutColor) );

    return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus VolumeNode::compute(const MPlug& plug, MDataBlock& block ) 
{ 
    if ((plug != aOutColor) && (plug.parent() != aOutColor) && 
		(plug != aOutTransparency) && (plug.parent() != aOutTransparency))
		return MS::kUnknownParameter;


    MFloatVector&  InputColor = block.inputValue( aColor ).asFloatVector();
    float Distance = block.inputValue( aInputValue ).asFloat();

    MFloatVector&  FarCamera = block.inputValue( aFarPointC ).asFloatVector();
    MFloatVector&  FarObject = block.inputValue( aFarPointO ).asFloatVector();
    MFloatVector&  FarWorld = block.inputValue( aFarPointW ).asFloatVector();
    MFloatVector&  PointCam = block.inputValue( aPointC ).asFloatVector();
    MFloatVector&  PointObj = block.inputValue( aPointO ).asFloatVector();
    MFloatVector&  PointWor = block.inputValue( aPointW ).asFloatVector();

    bool  Camera  = block.inputValue( aToggleCamera ).asBool();
    bool  Object  = block.inputValue( aToggleObject ).asBool();
    bool  World   = block.inputValue( aToggleWorld ).asBool();

    MFloatVector interval(0.0,0.0,0.0);
    if (Camera) {
		interval = FarCamera - PointCam;
    }
    if (Object) {
		interval = FarObject - PointObj;
    }
    if (World) {
		interval = FarWorld - PointWor;
    }

    double value = (interval[0]*interval[0]) +
             (interval[1]*interval[1]) + 
             (interval[2]*interval[2]);
    double dist = sqrt ( value );

    MFloatVector resultColor(0.0,0.0,0.0);
    if (dist <= Distance) {
	    resultColor[0] = InputColor[0];
	    resultColor[1] = InputColor[1];
	    resultColor[2] = InputColor[2];
    }

    // set ouput color attribute
    MDataHandle outColorHandle = block.outputValue( aOutColor );
    MFloatVector& outColor = outColorHandle.asFloatVector();
    outColor = resultColor;
    outColorHandle.setClean();

    // set output transparency
    MFloatVector transparency(resultColor[2],resultColor[2],resultColor[2]);
    MDataHandle outTransHandle = block.outputValue( aOutTransparency );
    MFloatVector& outTrans = outTransHandle.asFloatVector();
    outTrans = transparency;
    outTransHandle.setClean( );
 
    MDataHandle outAlphaHandle = block.outputValue( aOutAlpha );
    float& outAlpha = outAlphaHandle.asFloat();
    outAlpha = resultColor[2];
    outAlphaHandle.setClean( );

    return MS::kSuccess;
}


//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
	const MString UserClassify( "shader/volume" );

   MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.5", "Any");
   CHECK_MSTATUS( plugin.registerNode( "volumeNode", VolumeNode::id,
                         VolumeNode::creator, VolumeNode::initialize,
                         MPxNode::kDependNode, &UserClassify ) );

	return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus uninitializePlugin( MObject obj )
{
	MFnPlugin plugin( obj );
	CHECK_MSTATUS( plugin.deregisterNode( VolumeNode::id ) );

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
