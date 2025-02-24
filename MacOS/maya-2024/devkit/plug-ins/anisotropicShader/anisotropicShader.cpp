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

#include <math.h>

#include <maya/MFnPlugin.h>
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MIOStream.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatMatrix.h>

///////////////////////////////////////////////////////////////////////////////
//
// DESCRIPTION:    
// Produces dependency graph node AnisotropicShader

// This node modifies the specular highlight of a surface shader.

// The output attributes of the AnisotropicShader node are called "outColor" and "outTransparency". To use this shader, create an 
// AnisotropicShader node with a Shading Group or connect its output to a Shading Group's "SurfaceShader" attribute. 
//
///////////////////////////////////////////////////////////////////////////////

class anisotropicShaderNode : public MPxNode
{
    public:
	anisotropicShaderNode();
	~anisotropicShaderNode() override;

	MStatus compute( const MPlug&, MDataBlock& ) override;
	void    postConstructor() override;
    SchedulingType schedulingType() const override { return SchedulingType::kParallel; }

	static  void *  creator();
	static  MStatus initialize();

	//  Id tag for use with binary file format
	static const MTypeId id;

    private:
	MFloatVector calcHalfVector(const MFloatVector&,const MFloatVector&) const;
	static void setAttribute( );

	// Input attributes
	static MObject aColor;					// Surface color
	static MObject aDiffuseReflectivity;	// Diffuse Reflectivity
	static MObject aSpecularCoeff;			// Specular coefficient 
	static MObject aSpecColor;				// Specular color
	static MObject aInTransparency;			// Transparency

	static MObject aLightIntensity;			// Light Intensity 
	static MObject aLightDirection;			// Light direction vector

	static MObject aPointCamera;			// Position
	static MObject aNormalCamera;			// Surface normal
	static MObject aRayDirection;			// Ray direction

	// Light data
	static MObject aLightAmbient;
	static MObject aLightDiffuse;
	static MObject aLightSpecular;
	static MObject aLightShadowFraction;
	static MObject aPreShadowIntensity;
	static MObject aLightBlindData;
	static MObject aLightData;

	// anisotropic parameter
	static MObject aRoughness1;
	static MObject aRoughness2;
	static MObject aAxesVector;

	// matrix
	static MObject aMatrixOToW;
	static MObject aMatrixWToC;

	// Output attributes
	static MObject aOutColor;
	static MObject aOutTransparency;
};


// Static data
const MTypeId anisotropicShaderNode::id( 0x81014 );

// Attributes 
MObject    anisotropicShaderNode::aDiffuseReflectivity;
MObject    anisotropicShaderNode::aColor;
MObject    anisotropicShaderNode::aInTransparency;
MObject    anisotropicShaderNode::aNormalCamera;
MObject    anisotropicShaderNode::aLightData;
MObject    anisotropicShaderNode::aLightDirection;
MObject    anisotropicShaderNode::aLightIntensity; 
MObject    anisotropicShaderNode::aLightAmbient;
MObject    anisotropicShaderNode::aLightDiffuse;
MObject    anisotropicShaderNode::aLightSpecular;
MObject    anisotropicShaderNode::aLightShadowFraction;
MObject    anisotropicShaderNode::aPreShadowIntensity;
MObject    anisotropicShaderNode::aLightBlindData;
MObject    anisotropicShaderNode::aSpecularCoeff;
MObject    anisotropicShaderNode::aPointCamera;
MObject    anisotropicShaderNode::aSpecColor;
MObject    anisotropicShaderNode::aRoughness1;
MObject    anisotropicShaderNode::aRoughness2;
MObject    anisotropicShaderNode::aRayDirection;
MObject    anisotropicShaderNode::aAxesVector;
MObject    anisotropicShaderNode::aMatrixOToW;
MObject    anisotropicShaderNode::aMatrixWToC;
MObject    anisotropicShaderNode::aOutColor;
MObject    anisotropicShaderNode::aOutTransparency;

#define MAKE_INPUT(attr)						\
    CHECK_MSTATUS( attr.setKeyable(true) );  	\
	CHECK_MSTATUS( attr.setStorable(true) );	\
    CHECK_MSTATUS( attr.setReadable(true) ); 	\
	CHECK_MSTATUS( attr.setWritable(true) );

#define MAKE_OUTPUT(attr)						\
    CHECK_MSTATUS(attr.setKeyable(false) ); 	\
	CHECK_MSTATUS(attr.setStorable(false) );	\
    CHECK_MSTATUS(attr.setReadable(true) ); 	\
	CHECK_MSTATUS(attr.setWritable(false) );

void anisotropicShaderNode::postConstructor( )
{
}

anisotropicShaderNode::anisotropicShaderNode()
{
}

anisotropicShaderNode::~anisotropicShaderNode()
{
}

// creates an instance of the node
void* anisotropicShaderNode::creator()
{
    return new anisotropicShaderNode;
}

// initializes attribute information
// call by MAYA when this plug-in was loded.
//
MStatus anisotropicShaderNode::initialize()
{
    MFnNumericAttribute nAttr; 
    MFnLightDataAttribute lAttr;
    MFnMatrixAttribute mAttr;

    aMatrixOToW = mAttr.create( "matrixObjectToWorld", "mow",
								MFnMatrixAttribute::kFloat );
 
    CHECK_MSTATUS( mAttr.setStorable( false ) );
    CHECK_MSTATUS( mAttr.setHidden( true ) );

    aMatrixWToC = mAttr.create( "matrixWorldToEye", "mwc", 
								MFnMatrixAttribute::kFloat );
    CHECK_MSTATUS( mAttr.setStorable( false ) );
    CHECK_MSTATUS( mAttr.setHidden( true ) );

    aDiffuseReflectivity = nAttr.create( "diffuseReflectivity", "drfl",
										 MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setDefault(0.8f) );
    CHECK_MSTATUS( nAttr.setMin(0.0f) );
    CHECK_MSTATUS( nAttr.setMax(1.0f) );

    aColor = nAttr.createColor( "color", "c" );
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setDefault(0.0f, 0.58824f, 0.644f) );

    aNormalCamera = nAttr.createPoint( "normalCamera", "n" );
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setHidden(true) );

    aLightDirection = nAttr.createPoint( "lightDirection", "ld");
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightIntensity = nAttr.createColor( "lightIntensity", "li" );
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightAmbient = nAttr.create( "lightAmbient", "la",
								  MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightDiffuse = nAttr.create( "lightDiffuse", "ldf", 
								  MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightSpecular = nAttr.create( "lightSpecular", "ls",
								   MFnNumericData::kBoolean);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightShadowFraction = nAttr.create( "lightShadowFraction", "lsf",
										 MFnNumericData::kFloat);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aPreShadowIntensity = nAttr.create( "preShadowIntensity", "psi",
										MFnNumericData::kFloat);
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightBlindData = nAttr.createAddr( "lightBlindData", "lbld");
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(true) );
    CHECK_MSTATUS ( nAttr.setWritable(false) );

    aLightData = lAttr.create( "lightDataArray", "ltd",
                aLightDirection, 
                aLightIntensity, 
                aLightAmbient, 
                aLightDiffuse, 
                aLightSpecular,
                aLightShadowFraction,
                aPreShadowIntensity,
                aLightBlindData);
    CHECK_MSTATUS( lAttr.setArray(true) );
    CHECK_MSTATUS( lAttr.setStorable(false) );
    CHECK_MSTATUS( lAttr.setHidden(true) );
    CHECK_MSTATUS( lAttr.setDefault(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true, true,
					 false, 0.0f, 1.0f, NULL) );

    aSpecularCoeff = nAttr.create( "specularCoeff", "scf", 
								   MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setMin(0.0f) );
    CHECK_MSTATUS( nAttr.setMax(1.0f) );
    CHECK_MSTATUS( nAttr.setDefault(0.8f) );

    aPointCamera = nAttr.createPoint( "pointCamera", "pc" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setHidden(true) );

    // input transparency
    aInTransparency = nAttr.createColor( "transparency", "it" );
    MAKE_INPUT(nAttr);

    // ray direction
    aRayDirection = nAttr.createPoint( "rayDirection", "rd" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setHidden(true) );

    // specular color
    aSpecColor = nAttr.createColor( "specularColor","sc" );
    CHECK_MSTATUS( nAttr.setDefault( .5, .5, .5 ) );
    MAKE_INPUT(nAttr);

    // anisotropic parameters
    //
    aRoughness1 = nAttr.create( "roughness1", "rn1", MFnNumericData::kFloat);
    CHECK_MSTATUS( nAttr.setMin(0.0f) );
    CHECK_MSTATUS( nAttr.setMax(1.0f) );
    CHECK_MSTATUS( nAttr.setDefault(0.2f) );

    aRoughness2 = nAttr.create( "roughness2", "rn2", MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setMin(0.0f) );
    CHECK_MSTATUS( nAttr.setMax(1.0f) );
    CHECK_MSTATUS( nAttr.setDefault(0.4f) );

    aAxesVector = nAttr.createPoint( "axesVector", "av" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS( nAttr.setDefault( 0.0f, 1.0f, 0.0f ) );

    // output color
    aOutColor = nAttr.createColor( "outColor", "oc" );
	MAKE_OUTPUT(nAttr);

    // output transparency
    aOutTransparency = nAttr.createColor( "outTransparency", "ot" );
	MAKE_OUTPUT(nAttr);

    setAttribute();

    return MS::kSuccess;
}

void anisotropicShaderNode::setAttribute( )
{
    CHECK_MSTATUS( addAttribute( aDiffuseReflectivity ) );
    CHECK_MSTATUS( addAttribute( aColor ) );
    CHECK_MSTATUS( addAttribute( aInTransparency ) );
    CHECK_MSTATUS( addAttribute( aNormalCamera ) );
        
    // Only add the parent of the compound
    CHECK_MSTATUS( addAttribute( aLightData) );
    
    CHECK_MSTATUS( addAttribute( aSpecularCoeff) );
    CHECK_MSTATUS( addAttribute( aRayDirection ) );
    CHECK_MSTATUS( addAttribute( aPointCamera) );
    CHECK_MSTATUS( addAttribute( aSpecColor) );
    CHECK_MSTATUS( addAttribute( aRoughness1) );
    CHECK_MSTATUS( addAttribute( aRoughness2) );
    CHECK_MSTATUS( addAttribute( aAxesVector) );
    CHECK_MSTATUS( addAttribute( aMatrixOToW ) );
    CHECK_MSTATUS( addAttribute( aMatrixWToC ) );

    CHECK_MSTATUS( addAttribute( aOutColor) );
    CHECK_MSTATUS( addAttribute( aOutTransparency ) );

    CHECK_MSTATUS( attributeAffects( aDiffuseReflectivity, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightIntensity, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aColor, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aInTransparency, aOutColor ) );
    CHECK_MSTATUS( attributeAffects( aNormalCamera, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightData, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightSpecular, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightAmbient, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightDirection, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightDiffuse, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightShadowFraction, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aPreShadowIntensity, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aLightBlindData, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aSpecularCoeff, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aRayDirection, aOutColor ) );
    CHECK_MSTATUS( attributeAffects( aPointCamera, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aSpecColor, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aRoughness1,aOutColor) );
    CHECK_MSTATUS( attributeAffects( aRoughness2, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aAxesVector, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aMatrixOToW, aOutColor) );
    CHECK_MSTATUS( attributeAffects( aMatrixWToC, aOutColor) );

    CHECK_MSTATUS( attributeAffects( aInTransparency,  aOutTransparency ) );
}

#ifndef MIN
#define MIN(a,b) ( a<b?a:b )
#endif
//
//
MStatus anisotropicShaderNode::compute( const MPlug& plug, MDataBlock& block )
{
    if ((plug == aOutColor) || (plug.parent() == aOutColor))
	{
        MFloatVector resultColor(0.0,0.0,0.0);
        MFloatVector diffuseColor( 0.0,0.0,0.0 );
        MFloatVector specularColor( 0.0,0.0,0.0 );
        MFloatVector ambientColor( 0.0,0.0,0.0 );

        // get matrix
        MFloatMatrix& matrixOToW = block.inputValue( aMatrixOToW ).asFloatMatrix();
        MFloatMatrix& matrixWToC = block.inputValue( aMatrixWToC ).asFloatMatrix();

        // spin scratch around this vector (in object space )
        MFloatVector& A = block.inputValue( aAxesVector ).asFloatVector();
        A.normalize();

        // spin scratch around this vector (in world space )
        MFloatVector wa = A * matrixOToW;
        wa.normalize();

        // spin scratch around this vector (in camera space )
        MFloatVector ca = wa * matrixWToC;
        ca.normalize();

        MFloatVector& surfacePoint = block.inputValue( aPointCamera ).asFloatVector();

        // get sample surface shading parameters
        MFloatVector& N = block.inputValue( aNormalCamera ).asFloatVector();
        MFloatVector& surfaceColor = block.inputValue( aColor ).asFloatVector();

        float diffuseReflectivity = block.inputValue( aDiffuseReflectivity ).asFloat();
        float specularCoeff = block.inputValue( aSpecularCoeff ).asFloat();

        // get light list
        MArrayDataHandle lightData = block.inputArrayValue( aLightData );
        int numLights = lightData.elementCount();

        // iterate through light list and get ambient/diffuse values
        for( int count=0; count < numLights; count++ ) {
            MDataHandle currentLight = lightData.inputValue();

            MFloatVector& lightIntensity = 
                currentLight.child( aLightIntensity ).asFloatVector();
            MFloatVector& lightDirection = 
                currentLight.child( aLightDirection ).asFloatVector();

            // find ambient component
            if( currentLight.child(aLightAmbient).asBool()) {
                ambientColor[0] += lightIntensity[0] * surfaceColor[0];
                ambientColor[1] += lightIntensity[1] * surfaceColor[1];
                ambientColor[2] += lightIntensity[2] * surfaceColor[2];
            }

            float cosln = lightDirection * N;
            if( cosln > 0.0f ){ // illuminated!

                // find diffuse component
                if( currentLight.child(aLightDiffuse).asBool()) {
                
                    float cosDif = cosln * diffuseReflectivity;
                    diffuseColor[0] += lightIntensity[0] * cosDif * surfaceColor[0];
                    diffuseColor[1] += lightIntensity[1] * cosDif * surfaceColor[1];
                    diffuseColor[2] += lightIntensity[2] * cosDif * surfaceColor[2];
                }

                // find specular component
                if( currentLight.child( aLightSpecular).asBool()){

                    MFloatVector& rayDirection = block.inputValue( aRayDirection ).asFloatVector();
                    MFloatVector viewDirection = -rayDirection;
                    MFloatVector half = calcHalfVector( viewDirection, lightDirection );


                    // Beckmann function

                    MFloatVector nA;
                    if( fabs(1.0-fabs(N*ca)) <= 0.0001f ){
                        MFloatPoint oo( 0.0,0.0,0.0 );
                        MFloatPoint ow = oo * matrixOToW;
                        MFloatPoint oc = ow * matrixWToC;
                        MFloatVector origin( oc[0], oc[1], oc[2] );
                        nA = origin - surfacePoint;
                        nA.normalize();
                    }else{
                        nA = ca;
                    }

                    MFloatVector x = N ^ nA;
                    x.normalize();
                    MFloatVector y = N ^ x;
                    y.normalize();

                    MFloatVector azimuthH = N ^ half;
                    azimuthH = N ^ azimuthH;
                    azimuthH.normalize();

                    float cos_phai = x * azimuthH;
                    float sin_phai = 0.0;
                    if( fabs(1 - cos_phai*cos_phai) < 0.0001 ){
                        sin_phai = 0.0;
                    }else{
                        sin_phai = sqrtf( 1.0f - cos_phai*cos_phai );
                    }
                    double co = pow( (half * N), 4.0f );
                    double t = tan( acos(half*N) );
                    t *= -t;

                    float rough1 = block.inputValue( aRoughness1 ).asFloat();
                    float rough2 = block.inputValue( aRoughness2 ).asFloat();

                    double aaa = cos_phai / rough1;
                    double bbb = sin_phai / rough2;

                    t = t * ( aaa*aaa + bbb*bbb );

                    double D = pow( (1.0/((double)rough1*(double)rough2 * co)), t );

                    double aa = (2.0 * (N*half) * (N*viewDirection) ) / (viewDirection*half);
                    double bb = (2.0 * (N*half) * (N*lightDirection) ) / (viewDirection*half);
                    double cc = 1.0;
                    double G = 0.0;
                    G = MIN( aa, bb );
                    G = MIN( G, cc );

                    float s = (float) (D * G /
                            (double)((N*lightDirection) * (N*viewDirection)));
                    MFloatVector& specColor = block.inputValue( aSpecColor ).asFloatVector();
                    specularColor[0] += lightIntensity[0] * specColor[0] * 
                                            s * specularCoeff;
                    specularColor[1] += lightIntensity[1] * specColor[1] * 
                                            s * specularCoeff;
                    specularColor[2] += lightIntensity[2] * specColor[2] * 
                                            s * specularCoeff;
                }
            }

            if( !lightData.next() ){
                break;
            }
        }

        // result = specular + diffuse + ambient;
        resultColor = diffuseColor + specularColor + ambientColor;

        MFloatVector& transparency = block.inputValue( aInTransparency ).asFloatVector();
        resultColor[0] *= ( 1.0f - transparency[0] );
        resultColor[1] *= ( 1.0f - transparency[1] );
        resultColor[2] *= ( 1.0f - transparency[2] );

        // set ouput color attribute
        MDataHandle outColorHandle = block.outputValue( aOutColor );
        MFloatVector& outColor = outColorHandle.asFloatVector();
        outColor = resultColor;
        outColorHandle.setClean();
        block.setClean( plug );
    }
	else if ((plug == aOutTransparency) || (plug.parent() == aOutTransparency))
	{
        MFloatVector& tr = block.inputValue( aInTransparency ).asFloatVector();

        // set ouput color attribute
        MDataHandle outTransHandle = block.outputValue( aOutTransparency );
        MFloatVector& outTrans = outTransHandle.asFloatVector();
        outTrans = tr;
        block.setClean( plug );
    } else
		return MS::kUnknownParameter;

    return MS::kSuccess;
}

//
//
MFloatVector anisotropicShaderNode::calcHalfVector( 
	const MFloatVector& view, 
	const MFloatVector& light ) const
{
    MFloatVector H = (light + view) / 2.0;
    H.normalize();
    return H;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
   const MString UserClassify( "shader/surface" );

   MFnPlugin plugin( obj, "Tadashi Endo", "4.5", "Any");
   CHECK_MSTATUS( plugin.registerNode( "anisotropicShader", anisotropicShaderNode::id,
                         anisotropicShaderNode::creator, 
                         anisotropicShaderNode::initialize, 
                         MPxNode::kDependNode, &UserClassify ) );

   return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus uninitializePlugin( MObject obj )
{
   MFnPlugin plugin( obj );
   CHECK_MSTATUS( plugin.deregisterNode( anisotropicShaderNode::id ) );

   return MS::kSuccess;
}

