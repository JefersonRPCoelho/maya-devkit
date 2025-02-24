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

//
// This plug-in is based, in part, upon the folloing literature:
//
// 	 	Digital Seashells, M.B. Cortie,
// 	 	Computer and Graphics, Vol. 17, No. 1, pp. 79-84, 1993
// 
// 	 	_Shells_, S. Peter Dance,
// 	 	Harper Collins Publishers 1992, ISBN 0 7322 0067 9
// 
// 	 	Modeling Seashels, Deborah R. Fowler, Hans Meinhardt et al.
// 	 	Siggraph Proceedings 92, pp. 379-387
// 
// 	 	_The Algorithmic Beauty of Sea Shells_, Hans Meinhardt,
// 	 	Springer-Verlag Berlin Heidelberg 1995, ISBN 3 540 57842 0
//

////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
// 
// Produces the dependency graph node "shell". 
//
// This plug-in demonstrates how to generate a complex mesh object procedurally.
// It also demonstrates how to customize the attribute editor for a node.
//
// To use the plug-in:
//	
//	(1) Enter the MEL command "shell" to create a new shell node.
//		A mesh object will also be created to display the output.
//	(2) Open the attribute editor for the new shell node to see custom controls
//		for picking various sea shell presets. The attribute editor layout is
//		created by the file "AEShellTemplate.mel" and provides a complex
//		example of how to create an attribute editor template. 
//
////////////////////////////////////////////////////////////////////////

#include <maya/MPxNode.h> 

#include <maya/MString.h> 
#include <maya/MTypeId.h> 
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnPlugin.h>
#include <maya/MAngle.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>

#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MItMeshVertex.h>

#include <math.h>
#include <maya/MIOStream.h>

#define Rad(x)	((x)*FPI/180.0F)
#define Deg(x)	((x)*180.0F/FPI)
#define FPI 3.14159265358979323846264338327950288419716939937510582F 

#define McheckErr(stat,msg)         \
    if ( MS::kSuccess != stat ) {   \
        cerr << msg;                \
        return MS::kFailure;        \
    }


class shellNode : public MPxNode
{
public:
	shellNode();
	~shellNode() override; 

    MStatus   		compute( const MPlug& plug, MDataBlock& data ) override;

	static  void *          creator();
	static  MStatus         initialize();

public: 
	static	MTypeId		id;

	// Node attributes
	// ---------------

	// Main shape parameters
	//
	static	MObject alpha;		// Profile (Helico-spiral) param #1
	static	MObject	beta;		// Profile (Helico-spiral) param #2
	static	MObject	phi;		// Section Starting point
	static	MObject	my;		    // Section Slant
	static	MObject	omega;		// Section angle around Oz
	static	MObject	omin;		// Spiral start angle
	static	MObject	omax;		// Spiral end angle
	static	MObject	od;		    // Spiral angle step
	static	MObject	smin;		// Section start angle
	static	MObject	smax;		// Section end angle
	static	MObject	sd;		    // Section angle step
	static	MObject	A;		    // Distance of section from Z-axis
	static	MObject	a;		    // Section diameter #1
	static	MObject	b;		    // Section diameter #2
	static	MObject	scale;		// Overall scale
	
	// Nodule 1
	//
	static	MObject	P;			// Position on section
	static	MObject	L;			// Amplitude (extrusion) of nodule
	static	MObject	N;			// Nodules frequency on profile
	static	MObject	W1;			// Fatness of nodule
	static	MObject	W2;			// Fatness of nodule
	static	MObject	nstart;		// Starting point on spiral

	
	// Nodule 2
	// 
	static	MObject	P2;
	static	MObject	L2;
	static	MObject	N2;
	static	MObject	W12;
	static	MObject	W22;
	static	MObject	off2;
	static	MObject	nstart2;

	// Nodule 3
	// 
	static	MObject	P3;
	static	MObject	L3;
	static	MObject	N3;
	static	MObject	W13;
	static	MObject	W23;
	static	MObject	off3;
	static	MObject	nstart3;

	// Ribs
	//
	static	MObject	uamp;		// Section rib amplitude
	static	MObject	ufreq;		// Section rib frequency
	static	MObject	urib;		// Section rib/wave percent
	static	MObject	vamp;		// Profile rib amplitude
	static	MObject	vfreq;		// Profile rib frequency
	static	MObject	vrib;		// Profile rib/wave percent
    
    // Output mesh
    //
    static  MObject outMesh;

private:

	struct ShellParams {
		float	alpha;		// Profile (Helico-spiral) param #1
		float	beta;		// Profile (Helico-spiral) param #2
		float	phi;		// Section Starting point
		float	my;			// Section Slant
		float	omega;		// Section angle around Oz
		float	omin;		// Spiral start angle
		float	omax;		// Spiral end angle
		float	od;			// Spiral angle step
		float	smin;		// Section start angle
		float	smax;		// Section end angle
		float	sd;			// Section angle step
		float	A;			// Distance of section from Z-axis
		float	a;			// Section diameter #1
		float	b;			// Section diameter #2
		float	scale;		// Overall scale

		// Nodule 1
		// --------
		float	P;			// Position on section
		float	L;			// Amplitude (extrusion) of nodule
		float	N;			// Nodules frequency on profile
		float	W1;			// Fatness of nodule
		float	W2;			// Fatness of nodule
		float	nstart;		// Starting point on spiral

		// Nodule 2
		// --------
		float	P2;
		float	L2;
		float	N2;
		float	W12;
		float	W22;
		float	off2;
		float	nstart2;

		// Nodule 3
		// --------
		float	P3;
		float	L3;
		float	N3;
		float	W13;
		float	W23;
		float	off3;
		float	nstart3;

		// Ribs
		// ----
		float	uamp;		// Section rib amplitude
		float	ufreq;		// Section rib frequency
		float	urib;		// Section rib/wave percent
		float	vamp;		// Profile rib amplitude
		float	vfreq;		// Profile rib frequency
		float	vrib;		// Profile rib/wave percent
	};

	ShellParams shellParams;
	bool        redoTopology;
	bool        rebuild;

	// Precomputed shell points
	// 
	int	ni;
	int	nj;
	float	**pnts;

private:

	float GetFloatParameter( MObject node, MObject attr );
	float GetAngleParameter( MObject node, MObject attr );
	static void  addFloatParameter( MObject & attr, MString longName, 
									MString briefName, float attrDefault );
	static void  addAngleParameter( MObject & attr, MString longName, 
									MString briefName, float attrDefault );
	void  UpdateParameters(); 
	void  RedoTopology();
	void  Rebuild();
	float Nodules( float s, float o );
	float Ribs( float u, float v );
	void  Eval( float	*p, float o, float s );

};

MTypeId shellNode::id( 0x8000b );


shellNode::shellNode()
 :  rebuild( true ),
	redoTopology( true ),
    pnts( NULL ),
    ni( 0 ),
    nj( 0 )
{}

shellNode::~shellNode() {}

MStatus shellNode::compute( const MPlug& plug, MDataBlock& data )
{    
    MStatus returnStatus;
    int i, j;
    
    // Read updated input parameters
    //
	UpdateParameters();
    bool createNewMesh = redoTopology;
    RedoTopology();
    Rebuild();
    
    if( !pnts || ni<2 || nj<2 ) return MS::kSuccess;

    if (plug == outMesh) {  
        if( !pnts || ni<2 || nj<2 ) return MS::kSuccess;
        
		MDataHandle outputHandle = data.outputValue(outMesh, &returnStatus);
		McheckErr(returnStatus, "ERROR getting polygon data handle\n");
        MObject mesh = outputHandle.asMesh();
               
        if ( createNewMesh || mesh.isNull() ) {
            MFnMeshData dataCreator;
            MObject newOutputData = dataCreator.create(&returnStatus);
            McheckErr(returnStatus, "ERROR creating outputData");
            
		    MFloatPointArray vertices;

    	    // Build vertices array
    	    //
    	    for( j=0; j<nj; ++j ) {
		    	for( i=0; i<ni; ++i ) {
	        		const float *p= pnts[j] + 3*i;
	        		vertices.append( MFloatPoint(p[0],p[1],p[2]) );
		    	}
    	    }

    	    // Build poly vertex count array
    	    //
    	    MIntArray pcounts;
    	    i= (nj-1)*(ni-1);
    	    while( i-- ) {
                pcounts.append(4);
            }

    	    // Build poly connectivity array
    	    //
    	    MIntArray pconnect;
    	    for( j=0; j<nj-1; ++j ) {
		    	for( i=0; i<ni-1; ++i ) {
	        		int corner= i+j*ni;
	        		pconnect.append(corner);
	        		pconnect.append(corner+1);
	        		pconnect.append(corner+1+ni);
	        		pconnect.append(corner+ni);
		    	}
    	    }

    	    // Create some stuff needed by the mesh
   		    //
    	    MIntArray		fec;
    	    MDoubleArray	sp;
    	    MDoubleArray	tp;

    	    // Build maya poly object
    	    //
    	    MFnMesh meshFn;
    	    mesh= meshFn.create(
		    	nj*ni,				// number of vertices
		    	(nj-1)*(ni-1),		// number of polygons
		    	vertices,			// The points
		    	pcounts,			// # of vertex for each poly
		    	pconnect,			// Vertices index for each poly
		    	newOutputData,      // Dependency graph data object
		    	&returnStatus
    	    );

    	    // Update surface 
    	    //
            outputHandle.set(newOutputData);
        } else {
         	// The topology hasn't changed, so we can just set the points
            // in the existing mesh
            //
            MItMeshVertex vertIt( mesh, &returnStatus);
		    McheckErr(returnStatus, "ERROR creating iterator\n");
            
            for( j=0; j<nj; ++j ) {
		    	for( i=0; i<ni; ++i ) {
                    if ( vertIt.isDone() ) break;
	        		const float *p= pnts[j] + 3*i;
	        		vertIt.setPosition( MPoint(p[0],p[1],p[2]) );
                    vertIt.next();
		    	}
                if ( vertIt.isDone() ) break;
    	    }
        }
        data.setClean( plug );
    }   

	return MS::kSuccess;
}

void* shellNode::creator()
{
	return new shellNode();
}


//////////////////////
// Shell Algorithms //
//////////////////////

void shellNode::RedoTopology()
//
//  Description:
//      Adjust our data storage to reflect new parameters
//
{
    if( !redoTopology ) return;

	redoTopology = false;

    int	oldnj= pnts ? nj : 0;

    ni= 0;
    nj= 0;
    for( float s = shellParams.smin; 
		 s < shellParams.smax; 
		 s += shellParams.sd )  
	{
		ni++;	// Lazy
	}

    for( float o = shellParams.omin; 
		 o < shellParams.omax; 
		 o += shellParams.od )  
	{
		nj++;	// Lazy
	}

    if( nj<oldnj ) {
		for( int i = nj; i < oldnj; ++i ) {
			if( pnts[i] ) free( pnts[i] );
		}
	}

    if( nj!=oldnj) {
		float **new_data = static_cast<float**>(realloc (pnts, nj*sizeof(float*)));
		if (new_data == NULL) {
			delete pnts;
			pnts = NULL;
			return; // Allocation error, crash is imminent!
		} else {
			pnts = new_data;
		}
	}

    if( nj>oldnj ) {
		memset( pnts+oldnj, 0, (nj-oldnj)*sizeof(float*) );
	}
	
    for(int j=0;j<nj;++j) {
		pnts[j] = (float*) realloc(pnts[j],3*ni*sizeof(float));
	}
}

void shellNode::Rebuild()
//
//  Description:
//      Rebuild the mesh geometry given the new inputs
//
{
    if( !rebuild ) return;
    rebuild= 0;

    int		i;
    int		j;
    float	s;
    float	o;
    float	*p;

    o= shellParams.omin;
    for( j=0; j<nj; ++j )
    {
		s= shellParams.smin;
		p= pnts[j];
		for( i=0; i<ni; ++i )
		{
			Eval( p, o, s );
			s+=shellParams.sd;
			p+=3;
		}
		o+=shellParams.od;
    }
}

inline float SafeCot( float x )
{
    float s= sinf(x);
    return s ? cosf(x)/s : 0.0F;
}

inline float G( float a, float n )
{
    if( !n ) return n;
    float z= 2.0F*FPI;
    a*=n/z;
    return z/n*(a-floorf(0.5F+a));
}

float shellNode::Ribs( float u, float v )
{
	ShellParams & sp = shellParams;

    float zu=0.0F;
    if( sp.uamp )
    {
		zu= sp.uamp*cosf(2.0F*FPI*sp.ufreq*u);
		if( zu<0 ) zu*= (1.0F-2.0F*sp.urib);
    }

    float zv=0.0F;
    if( sp.vamp )
    {
		zv= sp.vamp*cosf(2.0F*FPI*sp.vfreq*v);
		if( zv<0 ) zv*= (1.0F-2.0F*sp.vrib);
    }

    return zu+zv;
}

float shellNode::Nodules( float	s, float o )
{
	ShellParams & sp = shellParams;

    float p1;
    float p2;
    float k=0.0F;
    float g=0.0F;

    if( sp.L && sp.N && o>=sp.nstart )
    {
		g= G(o,sp.N);
		p1= g/sp.W2;
		p2= (s-sp.P)/sp.W1;
		k= sp.L*expf( -4.0F*(p1*p1+p2*p2) );
    }

    if( sp.L2 && sp.N2 && o>=sp.nstart2 )
    {
		g= G(o+sp.off2,sp.N2); 
		p1= g/sp.W22;
		p2= (s-sp.P2)/sp.W12;
		k+= sp.L2*expf( -4.0F*(p1*p1+p2*p2) );
    }

    if( sp.L3 && sp.N3 && o>= sp.nstart3 )
    {
		g= G(o+sp.off3, sp.N3);
		p1= g/sp.W23;
		p2= (s-sp.P3)/sp.W13;
		k+= sp.L3*expf( -4.0F*(p1*p1+p2*p2) );
    }

    return k;
}

void shellNode::Eval( float	*p, float o, float s )
{
	ShellParams & sp = shellParams;

    float ss = sinf(s);
    float cs = cosf(s);
    float re = 1.0F/sqrtf(cs*cs/(sp.a*sp.a) 
               + ss*ss/(sp.b*sp.b));
    float sc = sp.scale*expf(o*SafeCot(sp.alpha));
    float csphi = cosf(s+sp.phi);
    float ssphi = sinf(s+sp.phi);
    float sbeta = sinf(sp.beta);
    float smy = sinf(sp.my);
    float r = re+Nodules(s,o)+Ribs(s,o);

    float x = sp.A*sbeta*cosf(o)		
              + r*csphi*cosf(o+sp.omega)	
              - r*smy*ssphi*sinf(o);

    float y = - sp.A*sbeta*sinf(o)	
		      - r*csphi*sinf(o+sp.omega)	
              - r*smy*ssphi*cosf(o);

    float z = - sp.A*cosf(sp.beta)		
              + r*ssphi*cosf(sp.my);

    p[0] =  x*sc;
    p[1] = -z*sc;
    p[2] =  y*sc;
}

/////////////////////////////////////
// Attribute Setup and Maintenance //
/////////////////////////////////////

	// Main shape parameters
	//
	MObject shellNode::alpha;		// Profile (Helico-spiral) param #1
	MObject	shellNode::beta;		// Profile (Helico-spiral) param #2
	MObject	shellNode::phi;		    // Section Starting point
	MObject	shellNode::my;		    // Section Slant
	MObject	shellNode::omega;		// Section angle around Z
	MObject	shellNode::omin;		// Spiral start angle
	MObject	shellNode::omax;		// Spiral end angle
	MObject	shellNode::od;		    // Spiral angle step
	MObject	shellNode::smin;		// Section start angle
	MObject	shellNode::smax;		// Section end angle
	MObject	shellNode::sd;		    // Section angle step
	MObject	shellNode::A;		    // Distance of section from Z-axis
	MObject	shellNode::a;		    // Section diameter #1
	MObject	shellNode::b;		    // Section diameter #2
	MObject	shellNode::scale;		// Overall scale
	
	// Nodule 1
	//
	MObject	shellNode::P;			// Position on section
	MObject	shellNode::L;			// Amplitude (extrusion) of nodule
	MObject	shellNode::N;			// Nodules frequency on profile
	MObject	shellNode::W1;			// Fatness of nodule
	MObject	shellNode::W2;			// Fatness of nodule
	MObject	shellNode::nstart;		// Starting point on spiral

	
	// Nodule 2
	// 
	MObject	shellNode::P2;
	MObject	shellNode::L2;
	MObject	shellNode::N2;
	MObject	shellNode::W12;
	MObject	shellNode::W22;
	MObject	shellNode::off2;
	MObject	shellNode::nstart2;

	// Nodule 3
	// 
	MObject	shellNode::P3;
	MObject	shellNode::L3;
	MObject	shellNode::N3;
	MObject	shellNode::W13;
	MObject	shellNode::W23;
	MObject	shellNode::off3;
	MObject	shellNode::nstart3;

	// Ribs
	//
	MObject	shellNode::uamp;		// Section rib amplitude
	MObject	shellNode::ufreq;		// Section rib frequency
	MObject	shellNode::urib;		// Section rib/wave percent
	MObject	shellNode::vamp;		// Profile rib amplitude
	MObject	shellNode::vfreq;		// Profile rib frequency
	MObject	shellNode::vrib;		// Profile rib/wave percent

    // Output mesh
    //
	MObject shellNode::outMesh;
    
void shellNode::addFloatParameter( MObject & attr, MString longName, 
								   MString briefName, float attrDefault )
//
//  Description:
//      Add a float input parameter to the node
//
{
	MStatus stat;
	MFnNumericAttribute nAttr;
	attr = nAttr.create( longName, briefName, MFnNumericData::kFloat, 
						 0.0, &stat );
	if ( stat != MS::kSuccess ) throw stat;

	stat = nAttr.setDefault  ( attrDefault );
	if ( stat != MS::kSuccess ) throw stat;

	stat = nAttr.setKeyable  ( true );
	if ( stat != MS::kSuccess ) throw stat;

	stat = nAttr.setCached   ( true );
	if ( stat != MS::kSuccess ) throw stat;

	stat = nAttr.setStorable ( true );
	if ( stat != MS::kSuccess ) throw stat;

	stat = addAttribute( attr );
	if ( stat != MS::kSuccess ) throw stat;
    
	stat = attributeAffects( attr, outMesh );
	if ( stat != MS::kSuccess ) throw stat;
}

void shellNode::addAngleParameter( MObject & attr, MString longName, 
								   MString briefName, float attrDefault )
//
//  Description:
//      Add an angle input parameter to the node
//
{
	MStatus stat;
	MFnUnitAttribute uAttr;

	MAngle defaultAngle( (double)attrDefault, MAngle::kDegrees );
    
	attr = uAttr.create( longName, briefName, defaultAngle, &stat );
	if ( stat != MS::kSuccess ) throw stat;

	stat = uAttr.setKeyable  ( true );
	if ( stat != MS::kSuccess ) throw stat;

	stat = uAttr.setCached   ( true );
	if ( stat != MS::kSuccess ) throw stat;

	stat = uAttr.setStorable ( true );
	if ( stat != MS::kSuccess ) throw stat;

	stat = addAttribute( attr );
	if ( stat != MS::kSuccess ) throw stat;
    
	stat = attributeAffects( attr, outMesh );
	if ( stat != MS::kSuccess ) throw stat;
}

MStatus shellNode::initialize()
//
//  Description
//      Set up node attributes
//
{
    MFnTypedAttribute   typedFn;
	MStatus			    stat;

    outMesh = typedFn.create( "outMesh", "o", MFnData::kMesh,
                               MObject::kNullObj, &stat ); 
    if ( MS::kSuccess != stat ) {
    	cerr << "ERROR creating animCube output attribute\n";
        return stat;
    }
    typedFn.setStorable(false);
    typedFn.setWritable(false);
    stat = addAttribute( outMesh );
    McheckErr(stat, "ERROR adding attribute");	
    
	try {
		addAngleParameter( alpha,   "profileParam1",           "pp1",   80.0F );
		addAngleParameter( beta,    "profileParam2",           "pp2",   90.0F );
		addAngleParameter( phi,     "sectionStartingPoint",    "ssp",    1.0F );
		addAngleParameter( my,      "sectionSlant",            "ss",     1.0F );
		addAngleParameter( omega,   "sectionAngleZ",           "saz",    1.0F );
		addAngleParameter( omin,    "spiralStartAngle",        "sps",    0.0F );
		addAngleParameter( omax,    "spiralEndAngle",          "spe", 1200.0F );
		addAngleParameter( od,      "spiralAngleStep",         "spa",    4.0F );
		addAngleParameter( smin,    "sectionStartAngle",       "ssa", -190.0F );
		addAngleParameter( smax,    "sectionEndAngle",         "sea",  190.0F );
		addAngleParameter( sd,      "sectionAngleStep",        "sas",   17.0F );

		addFloatParameter( A,       "distanceFromZ",           "dfz",    1.9F );
		addFloatParameter( a,       "sectionDiameter1",        "sd1",    1.0F );
		addFloatParameter( b,       "sectionDiameter2",        "sd2",    0.9F );
		addFloatParameter( scale,   "scale",                   "s",      0.03F );

		addAngleParameter( P,       "positionOnSection1",      "ps1",   10.0F );
		addFloatParameter( L,       "noduleAmplitude1",        "na1",    1.0F );
		addFloatParameter( N,       "noduleProfileFrequency1", "nf1",   15.0F );
		addAngleParameter( W1,      "noduleFatness11",         "f11",  100.0F );
		addAngleParameter( W2,      "noduleFatness21",         "f21",   20.0F );
		addAngleParameter( nstart,  "spiralStartingPoint1",    "sp1",    0.0F );

		addAngleParameter( P2,      "positionOnSection2",      "ps2",    0.0F );
		addFloatParameter( L2,      "noduleAmplitude2",        "na2",    0.0F );
		addFloatParameter( N2,      "noduleProfileFrequency2", "nf2",    0.0F );
		addAngleParameter( W12,     "noduleFatness12",         "f12",   30.0F );
		addAngleParameter( W22,     "noduleFatness22",         "f22",   30.0F );
		addAngleParameter( off2,    "noduleOffset2",           "no2",    0.0F );
		addAngleParameter( nstart2, "spiralStartingPoint2",    "sp2",    0.0F );

		addAngleParameter( P3,      "positionOnSection3",      "ps3",    0.0F );
		addFloatParameter( L3,      "noduleAmplitude3",        "na3",    0.0F );
		addFloatParameter( N3,      "noduleProfileFrequency3", "nf3",    0.0F );
		addAngleParameter( W13,     "noduleFatness13",         "f13",   30.0F );
		addAngleParameter( W23,     "noduleFatness23",         "f23",   30.0F );
		addAngleParameter( off3,    "noduleOffset3",           "no3",    0.0F );
		addAngleParameter( nstart3, "spiralStartingPoint3",    "sp3",    0.0F );

		addFloatParameter( uamp,    "sectionRibAmplitude",     "sra",    0.0F );
		addFloatParameter( ufreq,   "sectionRibFrequency",     "srf",    0.0F );
		addFloatParameter( urib,    "sectionRibWavePercent",   "srw",    0.0F );
		addFloatParameter( vamp,    "profileRibAmplitude",     "pra",    0.0F );
		addFloatParameter( vfreq,   "profileRibFrequency",     "prf",    0.0F );
		addFloatParameter( vrib,    "profileRibWavePercent",   "prw",    0.0F );
	} catch ( MStatus stat ) {
		fprintf(stderr,"Attribute Initialize Failed\n");
		return stat;
	}

	return MS::kSuccess;
}

inline float shellNode::GetFloatParameter( MObject node, MObject attr )
{
	MPlug plug( node, attr );
	float value;
	plug.getValue( value );
	return value;
}

inline float shellNode::GetAngleParameter( MObject node, MObject attr )
{
	MPlug plug( node, attr );
	MAngle angle;
	plug.getValue( angle );
	return (float)angle.asRadians();
}

#define UpdateFloatAttr(ATTR,TOPOLOGY)							\
    oldValue = shellParams. ATTR;							\
    shellParams. ATTR = GetFloatParameter( thisObj, ATTR );	\
    if ( shellParams. ATTR != oldValue ) {					\
	    rebuild = true;											\
		redoTopology = TOPOLOGY ? true : redoTopology;			\
	}

#define UpdateAngleAttr(ATTR,TOPOLOGY) 							\
    oldValue = shellParams. ATTR;							\
    shellParams. ATTR = GetAngleParameter( thisObj, ATTR );	\
    if ( shellParams. ATTR != oldValue ) {					\
	    rebuild = true;											\
		redoTopology = TOPOLOGY ? true : redoTopology;			\
	}

void shellNode::UpdateParameters( ) 
//
//  Description
//      Read all of the shell parameters and determine what has changed
//
{
	MObject thisObj = thisMObject();
	float oldValue;

	UpdateAngleAttr(alpha,false);
	UpdateAngleAttr(beta,false);
	UpdateAngleAttr(phi,false);
	UpdateAngleAttr(my,false);
	UpdateAngleAttr(omega,false);
	UpdateFloatAttr(A,false);
	UpdateFloatAttr(a,false);
	UpdateFloatAttr(b,false);
	UpdateFloatAttr(scale,false);

	UpdateAngleAttr(P,false);
	UpdateFloatAttr(L,false);
	UpdateFloatAttr(N,false);
	UpdateAngleAttr(W1,false);
	UpdateAngleAttr(W2,false);
	UpdateAngleAttr(nstart,false);

	UpdateAngleAttr(P2,false);
	UpdateFloatAttr(L2,false);
	UpdateFloatAttr(N2,false);
	UpdateAngleAttr(W12,false);
	UpdateAngleAttr(W22,false);
	UpdateAngleAttr(off2,false);
	UpdateAngleAttr(nstart2,false);
	UpdateAngleAttr(P3,false);
	UpdateFloatAttr(L3,false);
	UpdateFloatAttr(N3,false);
	UpdateAngleAttr(W13,false);
	UpdateAngleAttr(W23,false);
	UpdateAngleAttr(off3,false);
	UpdateAngleAttr(nstart3,false);
	UpdateFloatAttr(uamp,false);
	UpdateFloatAttr(ufreq,false);
	UpdateFloatAttr(urib,false);
	UpdateFloatAttr(vamp,false);
	UpdateFloatAttr(vfreq,false);
	UpdateFloatAttr(vrib,false);

	// These settings change the topology of the geometry
	//
	UpdateAngleAttr(omin,true);
	UpdateAngleAttr(omax,true);
	UpdateAngleAttr(od,true);
	UpdateAngleAttr(smin,true);
	UpdateAngleAttr(smax,true);
	UpdateAngleAttr(sd,true);
}


////////////////////////////
// Plug-in Initialization //
////////////////////////////

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerNode( "shell", shellNode::id, 
						 &shellNode::creator, &shellNode::initialize,
						 MPxNode::kDependNode );
	if (!status) {
		status.perror("registerNode");
		return status;
	}
	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( shellNode::id );
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
