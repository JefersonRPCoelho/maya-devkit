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

/////////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
//
// This plug-in demonstrates how to add user defined triangulation for meshes
// using the poly API class "MPxPolyTrg". 
//
// This node registers a user defined face triangulation function.
// After the function is registered, it can be used by any mesh in 
// the scene to do the triangulation (replace the mesh native 
// triangulation). In order for the mesh to use this function,
// an attribute on the mesh: "userTrg" has to be set to the name
// of the function. 
//	
// Different meshes may use different functions. Each of them 
// needs to be registered. The same node can provide more than 
// one function.  
//
// Example:
//	createNode polyTrgNode -n ptrg;
// 
//	polyPlane -w 1 -h 1 -sx 10 -sy 10 -ax 0 1 0 -tx 0 -ch 1 -n pp1;
//
//	select  -r pp1Shape;
//	setAttr pp1Shape.userTrg  -type "string" "triangulate";
//
/////////////////////////////////////////////////////////////////////////////

#include <maya/MIOStream.h>

// API stuff.
#include <maya/MFnPlugin.h>

// Node stuff.
#include <maya/MPxPolyTrg.h>
#include <maya/MFnMessageAttribute.h>

// Command stuff.
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MString.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MDagPath.h>

/////////////////////////////////////////////////////////////////////////////
//
// MACROS DECLARATION 
//
/////////////////////////////////////////////////////////////////////////////

#define MCHECKERR(stat,msg)  		\
	if ( MS::kSuccess != stat ) {   \
		return MS::kFailure;    	\
	}

#define EQUAL(a,b) ( (((a-b) > -0.00001) && ((a-b) < 0.00001)) ? 1 : 0 )
#define EQUAL2(a,b) ((EQUAL(a[0],b[0]) && EQUAL(a[1],b[1]) ) ? 1 : 0)
#define EQUAL3(a,b) ((EQUAL(a[0],b[0]) && EQUAL(a[1],b[1]) && EQUAL(a[2],b[2]) ) ? 1 : 0)

#define MCHECKERR_RETURN(stat,msg)  \
	if ( MS::kSuccess != stat ) {   \
		displayError( msg );		\
		fErrorCount = 1;			\
		return MS::kFailure;    	\
	}

#define MCHECKERR_BREAK(stat,msg)   \
	if ( MS::kSuccess != stat ) {   \
		displayError( msg );		\
		fErrorCount = 1;			\
		break;						\
	}


/////////////////////////////////////////////////////////////////////////////
//
// testPolyTrgNode node declaration
// 
/////////////////////////////////////////////////////////////////////////////

class polyTrgNode : public MPxPolyTrg
{
public:
    polyTrgNode() {};
    ~polyTrgNode() override;

    MStatus 	compute(const MPlug& plug, MDataBlock& data) override;
    void		postConstructor( ) override;
	bool		isAbstractClass( ) const override;

    static  void* 		creator();
    static  MStatus		initialize();

	// User triangulation - this is a triangulation per face.
	static  void 		triangulateFace( const float    *vert, 
									 	 const float 	*norm, 
									 	 const int		*loopSizes,
									 	 const int		nbLoops,
									 	 const int 		nbTrg,
									 	 unsigned short *trg );

public:
    static  MTypeId id;

private:
};

/////////////////////////////////////////////////////////////////////////////
//
// polyTrgNode implementation
//
/////////////////////////////////////////////////////////////////////////////

MTypeId polyTrgNode::id(0x101015);

polyTrgNode::~polyTrgNode() 
//
//	Description:
//		Destructor: unregister the triangulation function. 
//
{
	const char *_name = "triangulate";
	// Unregister the triangulation function.
    /* MStatus stat = */ unregisterTrgFunction( _name );
}

void * polyTrgNode::creator()
{
    return new polyTrgNode();
}

void polyTrgNode::postConstructor()
//
//	Description:
//		Constructor: register the triangulation function. 
//
{
    // Register the triangulation function.
	// The string given as a first parameter has to be
	// the same as the name given when setting the usrTrg
	// attribute on the mesh. See example above.
	// 
    /* MStatus stat = */ 
	const char *_name = "triangulate";
	registerTrgFunction( _name, polyTrgNode::triangulateFace );
}

bool polyTrgNode::isAbstractClass( ) const
{
	return false;
}

MStatus polyTrgNode::initialize()
{
    return MS::kSuccess;
}

MStatus polyTrgNode::compute(const MPlug& plug, MDataBlock& data)
{
	return MS::kSuccess;
}

void 
polyTrgNode::triangulateFace( 
	const float 	*vert, 			// I: face vertex position
	const float 	*norm, 			// I: face normals per vertex
	const int		*loopSizes,		// I: number of vertices per loop 
	const int		nbLoops,		// I: number of loops in the face	
	const int 		nbTrg,			// I: number of triangles to generate
	unsigned short *trg				// O: triangles - size = 3*nbTrg. 
									//    Note: this array is already allocated.
)
//
//  Description:
//		Triangulate a given face. Returns triangles given by 
//		the relative vertex ids. Example:
//	   		nbTrg = 2
//	   		trg: 0, 1, 2,  2, 3, 0
//
{
	// ======================================
	// Print the input.
	// ======================================
    cerr << "polyTrgNode::triangulate() - This is an API registered triangulation.\n";

	// Dump the data - this is a good example.
	cerr << "Numb Loops = " << nbLoops << "\n";
	int nbVert = 0;
	cerr << "Loop sizes: ";
	for (int i=0; i<nbLoops; i++ ) {
		nbVert += loopSizes[i];
		cerr << loopSizes[i] << " ";
	}
	cerr << "\n";
	cerr << "Numb Vert  = " << nbVert << "\n";

	cerr << "Vertices:\n";
   	for (int v=0; v<nbVert; v++ ) {
		cerr << vert[3*v] << " " << vert[3*v+1] << " " << vert[3*v+2] << " " <<"\n";
	}

	// Now triangulate.
	// ======================================
	
	cerr << " nbTrg = " << nbTrg << "\n";

	// ======================================
	// Generate a triangulation for this face.
	// ======================================
	unsigned short v0 = 0;
	unsigned short v1 = 1;
	unsigned short v2 = 2;
	for (int k=0; k<nbTrg; k++){
		trg[3*k] 	= v0;
		trg[3*k+1]  = v1;
		trg[3*k+2]  = v2;

		v1 = v2;
		v2 ++;
		if (v2 >= nbVert) {
			v2 = 0;
		}
	}
	// ======================================


	// ======================================
	// Print the result.
	// ======================================
	cerr << "Triangulation\n";
	for (int k1=0; k1<nbTrg; k1++){
		cerr << trg[3*k1] << " " << trg[3*k1+1] << " " << trg[3*k1+2] << "\n";
	}
	// ======================================
}




/////////////////////////////////////////////////////////////////////////////
//
// Node and command registration
//
/////////////////////////////////////////////////////////////////////////////

MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "MPxPlyTrg::Poly api example plug-in", "4.5", "Any");

	MStatus stat = plugin.registerNode("polyTrgNode",
                 						polyTrgNode::id,
                 						polyTrgNode::creator,
                 						polyTrgNode::initialize);
	return stat;
}

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
	MStatus stat = plugin.deregisterNode(polyTrgNode::id);
	return stat;
}
