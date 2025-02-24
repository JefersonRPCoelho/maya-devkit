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
#include <maya/MFloatVector.h>
#include <maya/MFloatPoint.h>
#include <maya/MFnPlugin.h>

///////////////////////////////////////////////////////////////////
// DESCRIPTION: 
//	Produces dependency graph node Flame 

//	This node is an example of a solid texture that uses turbulence and an axis to animate the texture's movement.

//	The output attributes of this node are "outColor" and "outAlpha."

//	To use this shader, create a Flame node and connect its output to an input of a surface/shader node such as Color. 
//
///////////////////////////////////////////////////////////////////



// Local functions
float Noise(float, float, float);
void  Noise_init();
static float Omega(int i, int j, int k, float t[3]);
static float omega(float);
static double turbulence(double u,double v,double w,int octaves);

#define PI                  3.14159265358979323846

#ifdef FLOOR
#undef FLOOR
#endif
#define FLOOR(x)            ((int)floorf(x))

#define TABLELEN            512
#define TLD2                256    // TABLELEN 

// Local variables
static int                  Phi[TABLELEN];
static char                 fPhi[TABLELEN];
static float                G[TABLELEN][3];


class Flame3D : public MPxNode
{
	public:
                    Flame3D();
            ~Flame3D() override;

    MStatus compute( const MPlug&, MDataBlock& ) override;
    SchedulingType schedulingType() const override { return SchedulingType::kParallel; }

    static  void *  creator();
    static  MStatus initialize();

	//  Id tag for use with binary file format
    static  MTypeId id;

	private:

	// Input attributes
    static MObject  aColorBase;
    static MObject  aColorFlame;
    static MObject  aRiseSpeed;
    static MObject  aFlickerSpeed;
    static MObject  aFlickerDeform;
    static MObject  aFlamePow;
    static MObject  aFlameFrame;
    static MObject  aRiseAxis;
    static MObject  aPlaceMat;
    static MObject  aPointWorld;

	// Output attributes
    static MObject  aOutAlpha;
    static MObject  aOutColor;
};

// Static data
MTypeId Flame3D::id(0x81016);

// Attributes
MObject  Flame3D::aColorBase;
MObject  Flame3D::aColorFlame;
MObject  Flame3D::aRiseSpeed;
MObject  Flame3D::aFlickerSpeed;
MObject  Flame3D::aFlickerDeform;
MObject  Flame3D::aFlamePow;
MObject  Flame3D::aFlameFrame;
MObject  Flame3D::aRiseAxis;
MObject  Flame3D::aPointWorld;
MObject  Flame3D::aPlaceMat;

MObject  Flame3D::aOutAlpha;
MObject  Flame3D::aOutColor;

#define MAKE_INPUT(attr)	\
    CHECK_MSTATUS(attr.setKeyable(true));  		\
	CHECK_MSTATUS(attr.setStorable(true));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
	CHECK_MSTATUS(attr.setWritable(true));

#define MAKE_OUTPUT(attr)	\
    CHECK_MSTATUS(attr.setKeyable(false)); 		\
	CHECK_MSTATUS(attr.setStorable(false));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
	CHECK_MSTATUS(attr.setWritable(false));

//
// DESCRIPTION:
///////////////////////////////////////////////////////
Flame3D::Flame3D()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
Flame3D::~Flame3D()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void * Flame3D::creator()
{
    return new Flame3D();
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus Flame3D::initialize()
{
    MFnMatrixAttribute mAttr;
    MFnNumericAttribute nAttr; 

	// Create input attributes
    aRiseSpeed = nAttr.create( "Rise", "rs", MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(0.1f));
    CHECK_MSTATUS(nAttr.setMin(0.0f));
    CHECK_MSTATUS(nAttr.setMax(1.0f));

    aFlickerSpeed = nAttr.create( "Speed", "s", MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(0.1f));
    CHECK_MSTATUS(nAttr.setMin(0.0f));
    CHECK_MSTATUS(nAttr.setMax(1.0f));

    aFlickerDeform = nAttr.create( "Flicker", "f", MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(0.5f));
    CHECK_MSTATUS(nAttr.setMin(0.0f));
    CHECK_MSTATUS(nAttr.setMax(1.0f));

    aFlamePow = nAttr.create( "Power", "pow", MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(1.0f));
    CHECK_MSTATUS(nAttr.setMin(0.0f));
    CHECK_MSTATUS(nAttr.setMax(1.0f));

    aFlameFrame = nAttr.create( "Frame", "fr", MFnNumericData::kFloat);
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(1.0f));
    CHECK_MSTATUS(nAttr.setMin(0.0f));
    CHECK_MSTATUS(nAttr.setMax(1000.0f));

    aRiseAxis = nAttr.createPoint( "Axis", "a");
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(0., 1., 0.));

    aColorBase = nAttr.createColor("ColorBase", "cg");
	MAKE_INPUT(nAttr);

    aColorFlame = nAttr.createColor("ColorFlame", "cb");
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setDefault(1., 1., 1.));
    
    aPlaceMat = mAttr.create("placementMatrix", "pm",
							 MFnMatrixAttribute::kFloat);
	MAKE_INPUT(mAttr);

	// Internal shading attribute, implicitely connected.
    aPointWorld = nAttr.createPoint("pointWorld", "pw");
	MAKE_INPUT(nAttr);
    CHECK_MSTATUS(nAttr.setHidden(true));

    // Create output attributes
    aOutColor  = nAttr.createColor("outColor",  "oc");
	MAKE_OUTPUT(nAttr);

    aOutAlpha = nAttr.create( "outAlpha", "oa", MFnNumericData::kFloat);
	MAKE_OUTPUT(nAttr);

	// Add the attributes here
    CHECK_MSTATUS(addAttribute(aColorBase));
    CHECK_MSTATUS(addAttribute(aColorFlame));
    CHECK_MSTATUS(addAttribute(aRiseSpeed));
    CHECK_MSTATUS(addAttribute(aFlickerSpeed));
    CHECK_MSTATUS(addAttribute(aFlickerDeform));
    CHECK_MSTATUS(addAttribute(aFlamePow));
    CHECK_MSTATUS(addAttribute(aFlameFrame));
    CHECK_MSTATUS(addAttribute(aRiseAxis));
    CHECK_MSTATUS(addAttribute(aPointWorld));
    CHECK_MSTATUS(addAttribute(aPlaceMat));

    CHECK_MSTATUS(addAttribute(aOutAlpha));
    CHECK_MSTATUS(addAttribute(aOutColor));

    // All input affect the output color and alpha
    CHECK_MSTATUS(attributeAffects (aColorBase, aOutColor));
    CHECK_MSTATUS(attributeAffects(aColorBase, aOutAlpha));

    CHECK_MSTATUS(attributeAffects (aColorFlame, aOutColor));
    CHECK_MSTATUS(attributeAffects(aColorFlame, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aRiseSpeed, aOutColor));
    CHECK_MSTATUS(attributeAffects(aRiseSpeed, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aFlickerSpeed, aOutColor));
    CHECK_MSTATUS(attributeAffects(aFlickerSpeed, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aFlickerDeform, aOutColor));
    CHECK_MSTATUS(attributeAffects(aFlickerDeform, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aFlamePow, aOutColor));
    CHECK_MSTATUS(attributeAffects(aFlamePow, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aFlameFrame, aOutColor));
    CHECK_MSTATUS(attributeAffects(aFlameFrame, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aRiseAxis, aOutColor));
    CHECK_MSTATUS(attributeAffects(aRiseAxis, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aPointWorld, aOutColor));
    CHECK_MSTATUS(attributeAffects (aPointWorld, aOutAlpha));

    CHECK_MSTATUS(attributeAffects(aPlaceMat, aOutColor));
    CHECK_MSTATUS(attributeAffects(aPlaceMat, aOutAlpha));

    return MS::kSuccess;
}


///////////////////////////////////////////////////////
// DESCRIPTION:
// This function gets called by Maya to evaluate the texture.
//
///////////////////////////////////////////////////////

MStatus Flame3D::compute(const MPlug& plug, MDataBlock& block) 
{ 
	// outColor or individial R, G, B channel, or alpha
    if((plug != aOutColor) && (plug.parent() != aOutColor) && 
	   (plug != aOutAlpha))
       return MS::kUnknownParameter;

	float3 & worldPos = block.inputValue( aPointWorld).asFloat3();
    const MFloatMatrix& mat = block.inputValue(aPlaceMat).asFloatMatrix();
    const MFloatVector& cBase = block.inputValue(aColorBase).asFloatVector();
    const MFloatVector& cFlame=block.inputValue(aColorFlame).asFloatVector();
    const MFloatVector& axis = block.inputValue( aRiseAxis ).asFloatVector();
    const float rise_speed = block.inputValue( aRiseSpeed ).asFloat();
    const float flicker_speed = block.inputValue( aFlickerSpeed ).asFloat();
    const float dscale = block.inputValue( aFlickerDeform ).asFloat();
    const float frame = block.inputValue( aFlameFrame ).asFloat();
    const float power = block.inputValue( aFlamePow ).asFloat();
    
	MFloatPoint q(worldPos[0], worldPos[1], worldPos[2]);
	q *= mat;								// Convert into solid space

	// Offset texture coord along the RiseAxis
    float rise_distance = -1.0f * rise_speed * frame;
    float u,v,w;
    u = q.x + ( rise_distance * axis[0]);
    v = q.y + ( rise_distance * axis[1]);
    w = q.z + ( rise_distance * axis[2]);

	// Generate a displaced point by moving along the
	// displacement vector (currently the 1,1,1 vector)
	// based on flicker speed.
    float dist = flicker_speed * frame;
    float au, av, aw;
    au = u + dist;
    av = v + dist;
    aw = w + dist;

	// Calculate 3 noise values
    float ascale = Noise(au,av,aw);
    // float bscale = Noise(au,-av,aw);
    // float cscale = Noise(-au,av,-aw);

	// add this noise as a vector to the texture coordinates
	// (since we are only calculating one value, the
	// displacement will be along the 1 1 1 vector ... this
	// displacement generates the "flicker" movement as the
	// value moves around the texture coordinate

    u += ascale * dscale;
    v += ascale * dscale;
    w += ascale * dscale;

	// Calculate a turbulence value for this point

    float scalar = (float) (turbulence(u, v, w, 3) + 0.5);

	// convert scalar into a point on the color curve

    if (power != 1) scalar = powf (scalar, power);

    MFloatVector resultColor;
    if (scalar >= 1)
		resultColor = cFlame;
    else if (scalar < 0) 
		resultColor = cBase;
	else
		resultColor = ((cFlame-cBase)*scalar) + cBase;

    MDataHandle outHandle = block.outputValue( aOutColor );
    MFloatVector & outColor = outHandle.asFloatVector();
    outColor = resultColor;
    outHandle.setClean();

    outHandle = block.outputValue(aOutAlpha);
    outHandle.asFloat() = scalar;
    outHandle.setClean();

    return MS::kSuccess;
}

//////////////////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
    const MString UserClassify( "texture/3d" );

    MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any" );
    CHECK_MSTATUS( plugin.registerNode( "flame", Flame3D::id, 
	    Flame3D::creator, Flame3D::initialize,
	    MPxNode::kDependNode, &UserClassify) );

    Noise_init();
    
    return MS::kSuccess;
}

// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus uninitializePlugin( MObject obj )
{
    MFnPlugin plugin( obj );
    CHECK_MSTATUS( plugin.deregisterNode( Flame3D::id ) );

    return MS::kSuccess;
}

//
//  REFERENCES:
//      Perlin, K. An Image Synthesizer, Computer Graphics, 
//      Vol. 19, No. 3, July 1985.
//
//      Perlin, K., Hoffert, E.M., Hypertexture, Computer Graphics, 
//      Vol. 23, No. 3, July 1989.
//

float Noise(float u, float v, float w)
{
    int         i;
    int         j;
    int         k;
    int         ul;
    int         vl;
    int         wl;
    float       ans;
    float       t[3];

    ans = 0.0;
    ul  = FLOOR(u);
    vl  = FLOOR(v);
    wl  = FLOOR(w);

    for(i = ul + 1; i >= ul; i--)
    {  
		t[0] = u - i;
		for(j = vl + 1; j >= vl; j--)
        {   
			t[1] = v - j;
			for(k = wl + 1; k >= wl; k--)
            {
				t[2] = w - k;
				ans += Omega(i, j, k, t);
            }
        }
    }

    return ans;
}

static float Omega(int i, int j, int k, float t[3])
{
    int ct;

    ct = Phi[((i + 
         Phi[((j + 
         Phi[(k%TLD2)+TLD2]) % TLD2) + TLD2]) % TLD2) + TLD2];

    return omega(t[0]) * omega(t[1]) * omega(t[2]) *
		( G[ct][0]*t[0] + G[ct][1]*t[1] + G[ct][2]*t[2] );
}

static float omega(float t)
{
    t  = fabsf(t);
    return (t * (t * (t * (float)2.0 - (float)3.0))) + (float)1.0;
}

void Noise_init()
{
    int i;
    float u, v, w, s, len;
    static int first_time = 1;

    if (first_time)
        first_time = 0;
    else
        return;

    (void)srand48(0l);

    for(i = 0; i < TABLELEN; i++)
        fPhi[i] = 0;

    for(i = 0; i < TABLELEN; i++) {
		Phi[i] = lrand48() % TABLELEN;

        if (fPhi[Phi[i]])
            i--;
        else
            fPhi[Phi[i]] = 1;
    }
    for(i = 0; i < TABLELEN; i++) {
		u = (float) (2.0 * drand48() - 1.0);
		v = (float) (2.0 * drand48() - 1.0);
		w = (float) (2.0 * drand48() - 1.0);
		if((s = u*u + v*v + w*w) > 1.0) 
        {   i--;
	    continue;
		}
        else
			if (s == 0.0)
			{   i--;
			continue;
			}

		len = 1.0f / sqrtf(s);
		G[i][0] = u * len;
		G[i][1] = v * len;
		G[i][2] = w * len;
    }
}

static double turbulence(double u,double v,double w,int octaves)
{
	double s,t;
	s = 1.0;
	t = 0.0;

	while (octaves--) {
		t += Noise((float)u, (float)v, (float)w)*s;
		s *= 0.5;
		u*=2.0; v*=2.0; w*=2.0;
	}
	return t;
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
