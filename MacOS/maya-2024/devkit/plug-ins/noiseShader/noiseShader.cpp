#include <math.h>
#include <stdlib.h>
#include <maya/MPxNode.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h> 
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatMatrix.h>
#include <maya/MFnPlugin.h>

///////////////////////////////////////////////////////////////////
// DESCRIPTION: 
// Produces dependency graph node SolidNoise

// This node is an example of a 3d texture.

// The output attribute of the SolidNoise node is called "outColor". 
//
///////////////////////////////////////////////////////////////////


class noise3 : public MPxNode
{
    public:

    noise3();
    ~noise3() override;

    MStatus compute( const MPlug&, MDataBlock& ) override;
    SchedulingType schedulingType() const override { return SchedulingType::kParallel; }

    static  void *  creator();
    static  MStatus initialize();

	//  Id tag for use with binary file format
    static MTypeId id;

	private:

	static void init();
	static float pnoise3( MFloatPoint& vec );
	static float pnoise3( float vx, float vy, float vz );

	// Input attributes
	static MObject aColor1;
	static MObject aColor2;
    static MObject aScale;
    static MObject aBias;
	static MObject aPlaceMat;
	static MObject aPointWorld;

	// Output attributes
    static MObject          aOutColor;
    static MObject          aOutAlpha;
};

// Static data
MTypeId noise3::id( 0x8100a );

// Attributes
MObject noise3::aColor1;
MObject noise3::aColor2;
MObject noise3::aPlaceMat;
MObject noise3::aPointWorld;
MObject noise3::aScale;
MObject noise3::aBias;
 
MObject noise3::aOutColor;
MObject noise3::aOutAlpha;

#define MAKE_INPUT(attr)								\
    CHECK_MSTATUS ( attr.setKeyable(true) );   \
	CHECK_MSTATUS ( attr.setStorable(true) );		\
    CHECK_MSTATUS ( attr.setReadable(true) );  \
	CHECK_MSTATUS ( attr.setWritable(true) );

#define MAKE_OUTPUT(attr)								\
    CHECK_MSTATUS ( attr.setKeyable(false) ); \
	CHECK_MSTATUS ( attr.setStorable(false) );	\
    CHECK_MSTATUS ( attr.setReadable(true) ); \
	CHECK_MSTATUS ( attr.setWritable(false) );

noise3::noise3()
{
}

noise3::~noise3()
{
}

// creates an instance of the node
void * noise3::creator()
{
    return new noise3();
}

// initializes attribute information
MStatus noise3::initialize()
{
    MFnMatrixAttribute mAttr; 
    MFnNumericAttribute nAttr; 

	// Create input attributes

	aColor1 = nAttr.createColor("color1", "c1");
	MAKE_INPUT(nAttr);
	CHECK_MSTATUS ( nAttr.setDefault(0., .58824, .644) );		// Light blue

	aColor2 = nAttr.createColor("color2", "c2");
	MAKE_INPUT(nAttr);
	CHECK_MSTATUS ( nAttr.setDefault(1., 1., 1.) );			// White

    aScale = nAttr.create( "scale", "s", MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault( 1. ) );

    aBias = nAttr.create( "bias", "b", MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);

    aPlaceMat = mAttr.create("placementMatrix", "pm",
							 MFnMatrixAttribute::kFloat);
    MAKE_INPUT(mAttr);

	// Implicit shading network attributes

    aPointWorld = nAttr.createPoint("pointWorld", "pw");
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    // Create output attributes

    aOutColor = nAttr.createColor("outColor", "oc");
	MAKE_OUTPUT(nAttr);

    aOutAlpha = nAttr.create( "outAlpha", "oa", MFnNumericData::kFloat);
	MAKE_OUTPUT(nAttr);

    // Add the attributes here
    CHECK_MSTATUS ( addAttribute(aColor1) );
    CHECK_MSTATUS ( addAttribute(aColor2) );
    CHECK_MSTATUS ( addAttribute(aScale) );
    CHECK_MSTATUS ( addAttribute(aBias) );
    CHECK_MSTATUS ( addAttribute(aPointWorld) );
    CHECK_MSTATUS ( addAttribute(aPlaceMat) );

    CHECK_MSTATUS ( addAttribute(aOutColor) );
    CHECK_MSTATUS ( addAttribute(aOutAlpha) );

    // All input affect the output color and alpha
    CHECK_MSTATUS ( attributeAffects (aColor1, aOutColor) );
    CHECK_MSTATUS ( attributeAffects (aColor1, aOutAlpha) );

    CHECK_MSTATUS ( attributeAffects (aColor2, aOutColor) ) ;
    CHECK_MSTATUS ( attributeAffects (aColor2, aOutAlpha) );

    CHECK_MSTATUS ( attributeAffects (aScale, aOutAlpha) );
    CHECK_MSTATUS ( attributeAffects (aScale, aOutColor) );

    CHECK_MSTATUS ( attributeAffects (aBias, aOutAlpha) );
    CHECK_MSTATUS ( attributeAffects (aBias, aOutColor) ); 

    CHECK_MSTATUS ( attributeAffects (aPointWorld, aOutColor) );
    CHECK_MSTATUS ( attributeAffects (aPointWorld, aOutAlpha) );

    CHECK_MSTATUS ( attributeAffects (aPlaceMat, aOutColor) );
    CHECK_MSTATUS ( attributeAffects (aPlaceMat, aOutAlpha) );

    return MS::kSuccess;
}

/* Ken Perlin */

#define DOT(a,b) (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) 
#define B 256 
static int p[B +B +2]; 
static float g[B + B + 2][3]; 
static int start = 1; 

#define setup(i,b0,b1,r0,r1) t = i + 10000.0f; b0 = ((int)t) & (B-1);  b1 = (b0+1) & (B-1);  r0 = t - (int)t;  r1 = r0 - 1.0f; 

inline float noise3::pnoise3( MFloatPoint& vec ) 
{
	return pnoise3( vec.x, vec.y, vec.z );
}

float noise3::pnoise3(float vx, float vy, float vz) 
{ 
	int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11; 
	float rx0, rx1, ry0, ry1, rz0, rz1, *q, sx, sy, sz, a, b, c, d, t, u, v; 
	int i, j; 

	if (start) 
	{ 
		start = 0; 
		init(); 
	} 
	setup(vx, bx0,bx1, rx0,rx1); 
	setup(vy, by0,by1, ry0,ry1); 
	setup(vz, bz0,bz1, rz0,rz1); 
	i = p[ bx0 ]; 
	j = p[ bx1 ]; 
	b00 = p[ i + by0 ]; 
	b10 = p[ j + by0 ]; 
	b01 = p[ i + by1 ]; 
	b11 = p[ j + by1 ];

	#define at(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] ) 
	#define s_curve(t) ( t * t * (3.0f - 2.0f * t) ) 
	#define lerp(t, a, b) ( a + t * (b - a) ) 

	sx = s_curve(rx0); 
	sy = s_curve(ry0); 
	sz = s_curve(rz0); 
	q = g[ b00 + bz0 ] ; 
	u = at(rx0,ry0,rz0); 
	q = g[ b10 + bz0 ] ; 
	v = at(rx1,ry0,rz0); 
	a = lerp(sx, u, v); 
	q = g[ b01 + bz0 ] ; 
	u = at(rx0,ry1,rz0); 
	q = g[ b11 + bz0 ] ; 
	v = at(rx1,ry1,rz0); 
	b = lerp(sx, u, v); 
	c = lerp(sy, a, b);						// interpolate in y at lo x
	q = g[ b00 + bz1 ] ; 
	u = at(rx0,ry0,rz1); 
	q = g[ b10 + bz1 ] ; 
	v = at(rx1,ry0,rz1); 
	a = lerp(sx, u, v); 
	q = g[ b01 + bz1 ] ; 
	u = at(rx0,ry1,rz1); 
	q = g[ b11 + bz1 ] ; 
	v = at(rx1,ry1,rz1); 
	b = lerp(sx, u, v); 
	d = lerp(sy, a, b);						// interpolate in y at hi x
	return 1.5f * lerp(sz, c, d);			// interpolate in z
}

void noise3::init() 
{
	int i, j, k; 
	float v[3], s; 
 
	// Create an array of random gradient vectors uniformly on the
	// unit sphere
	srandom(1); 
	for (i = 0 ; i < B ; i++) 
	{ 
		do 
		{ 
			// Choose uniformly in a cube
			for (j=0 ; j<3 ; j++) 
				v[j] = (float)((random() % (B + B)) - B) / B; 
			s = DOT(v,v); 
		} while (s > 1.0);					// If not in sphere try again
		s = sqrtf(s); 
		for (j = 0 ; j < 3 ; j++)			// Else normalize
			g[i][j] = v[j] / s; 
	} 

	// Create a pseudorandom permutation of [1..B]
	for (i = 0 ; i < B ; i++) 
		p[i] = i; 
	for(i=B ;i >0 ;i -=2)
	{ 
		k = p[i]; 
		p[i] = p[j = random() % B]; 
		p[j] = k; 
	} 
	// Extend g and p arrays to allow for faster indexing
	for(i=0 ;i <B +2 ;i++) 
	{ 
		p[B + i] = p[i]; 
		for (j = 0 ; j < 3 ; j++) 
			g[B + i][j] = g[i][j]; 
	}
}

//
// This function gets called by Maya to evaluate the texture.
//
MStatus noise3::compute(const MPlug& plug, MDataBlock& block) 
{
	// outColor or individial R, G, B channel, or alpha
    if((plug != aOutColor) && (plug.parent() != aOutColor) && 
	   (plug != aOutAlpha))
		return MS::kUnknownParameter;

	MFloatVector resultColor;
	MFloatVector & col1 = block.inputValue(aColor1).asFloatVector();
	MFloatVector & col2 = block.inputValue(aColor2).asFloatVector();
	
	float3 & worldPos = block.inputValue( aPointWorld ).asFloat3();
	MFloatMatrix& mat = block.inputValue( aPlaceMat ).asFloatMatrix();
	float& sc = block.inputValue( aScale ).asFloat();
	float& bi = block.inputValue( aBias ).asFloat();

	MFloatPoint solidPos(worldPos[0], worldPos[1], worldPos[2]);
	solidPos *= mat;						// Convert into solid space

	float val = fabsf( pnoise3( solidPos ) * sc + bi );
	if (val < 0.) val = 0.;
	if (val > 1.) val = 1.;
	resultColor = col1 * val + col2*(1-val);

	// Set output color attribute
	MDataHandle outColorHandle = block.outputValue( aOutColor );
	MFloatVector& outColor = outColorHandle.asFloatVector();
	outColor = resultColor;
	outColorHandle.setClean();

	MDataHandle outAlphaHandle = block.outputValue( aOutAlpha );
	float& outAlpha = outAlphaHandle.asFloat();
	outAlpha = val;
	outAlphaHandle.setClean();

    return MS::kSuccess;
}


MStatus initializePlugin( MObject obj )
{
    const MString UserClassify( "texture/3d" );

    MFnPlugin plugin( obj, PLUGIN_COMPANY, "4.5", "Any");
    CHECK_MSTATUS ( plugin.registerNode( "solidNoise", noise3::id, 
                         &noise3::creator, &noise3::initialize,
                         MPxNode::kDependNode, &UserClassify ) );

    return MS::kSuccess;
}

MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin( obj );
    CHECK_MSTATUS ( plugin.deregisterNode( noise3::id ) );

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
