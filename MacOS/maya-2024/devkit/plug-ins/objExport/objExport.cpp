//-
// Copyright 2020 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//+

#include <string.h> 
#include <sys/types.h>
#include <maya/MStatus.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshEdge.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MObjectArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MDistance.h>
#include <maya/MIntArray.h>
#include <maya/MIOStream.h>

#if defined  (__APPLE__)
extern "C" Boolean createMacFile (const char *fileName, FSRef *fsRef, long creator, long type);
#endif

#define NO_SMOOTHING_GROUP      -1
#define INITIALIZE_SMOOTHING    -2
#define INVALID_ID              -1

//
// Edge info structure
//
typedef struct EdgeInfo {
    int                 polyIds[2]; // Id's of polygons that reference edge
    int                 vertId;     // The second vertex of this edge
    struct EdgeInfo *   next;       // Pointer to next edge
    bool                smooth;     // Is this edge smooth
} * EdgeInfoPtr;


//////////////////////////////////////////////////////////////
class ObjTranslator : public MPxFileTranslator {
public:
                    ObjTranslator () {};
    virtual         ~ObjTranslator () {};
    static void*    creator();

    MStatus         reader ( const MFileObject& file,
                             const MString& optionsString,
                             FileAccessMode mode);

    MStatus         writer ( const MFileObject& file,
                             const MString& optionsString,
                             FileAccessMode mode );
    bool            haveReadMethod () const;
    bool            haveWriteMethod () const;
    MString         defaultExtension () const;
    MFileKind       identifyFile ( const MFileObject& fileName,
                                   const char* buffer,
                                   short size) const;
private:
    void            outputSetsAndGroups    ( MDagPath&, int, bool, int );
    MStatus         OutputPolygons( MDagPath&, MObject& );
    MStatus         exportSelected();
    MStatus         exportAll();
	void		    initializeSetsAndLookupTables( bool exportAll );
	void		    freeLookupTables();
	bool 		    lookup( MDagPath&, int, int, bool );
	void            setToLongUnitName( const MDistance::Unit&, MString& );
	void			recFindTransformDAGNodes( MString&, MIntArray& );
	

    // Edge lookup methods
    //
    void            buildEdgeTable( MDagPath& );
    void            addEdgeInfo( int, int, bool );
    EdgeInfoPtr     findEdgeInfo( int, int );
    void            destroyEdgeTable();
    bool            smoothingAlgorithm( int, MFnMesh& );

private:
    // counters
    int v,vt,vn;
    // offsets
    int voff,vtoff,vnoff;
    // options
    bool groups, ptgroups, materials, smoothing, normals;

    FILE *fp;
	
	// Keeps track of all sets.
	//
	int numSets;
	MObjectArray *sets;
	
	// Keeps track of all objects and components.
	// The Tables are used to mark which sets each 
	// component belongs to.
	//
	MStringArray *objectNames;
	
	bool **polygonTablePtr;
	bool **vertexTablePtr;
	bool * polygonTable;
	bool * vertexTable;
	bool **objectGroupsTablePtr;
	
	
	// Used to determine if the last set(s) written out are the same
	// as the current sets to be written. We don't need to write out
	// sets unless they change between components. Same goes for
	// materials.
	//
	MIntArray *lastSets;
	MIntArray *lastMaterials;
	
	// We have to do 2 dag iterations so keep track of the
	// objects found in the first iteration by this index.
	//
	int objectId;
	int objectCount;
    
    // Edge lookup table (by vertex id) and smoothing group info
    //
    EdgeInfoPtr *   edgeTable;
    int *           polySmoothingGroups;
    int             edgeTableSize;
    int             nextSmoothingGroup;
    int             currSmoothingGroup;
    bool            newSmoothingGroup;

	// List of names of the mesh shapes that we export from maya
	MStringArray	objectNodeNamesArray;

	// Used to keep track of Maya groups (transform DAG nodes) that
	// contain objects being exported
	MStringArray	transformNodeNameArray;
};

//////////////////////////////////////////////////////////////

const char *const objOptionScript = "objExportOptions";
const char *const objDefaultOptions =
    "groups=1;"
    "ptgroups=1;"
    "materials=1;"
    "smoothing=1;"
    "normals=1;"
    ;

//////////////////////////////////////////////////////////////

void* ObjTranslator::creator()
{
    return new ObjTranslator();
}

//////////////////////////////////////////////////////////////

MStatus ObjTranslator::reader ( const MFileObject& file,
                                const MString& options,
                                FileAccessMode mode)
{
    fprintf(stderr, "ObjTranslator::reader called in error\n");
    return MS::kFailure;
}


MStatus ObjTranslator::writer ( const MFileObject& file,
                                const MString& options,
                                FileAccessMode mode )

{
    MStatus status;
    
	MString mname = file.expandedFullName(), unitName;
    

    const char *fname = mname.asChar();
    fp = fopen(fname,"w");

    if (fp == NULL)
    {
        cerr << "Error: The file " << fname << " could not be opened for writing." << endl;
        return MS::kFailure;
    }

    // Options
    //
    groups      = true; // write out facet groups
    ptgroups    = true; // write out vertex groups
    materials   = true; // write out shading groups
    smoothing   = true; // write out facet smoothing information
    normals     = true; // write out normal table and facet normals
    
  if (options.length() > 0) {
        int i, length;
        // Start parsing.
        MStringArray optionList;
        MStringArray theOption;
        options.split(';', optionList); // break out all the options.

		length = optionList.length();
		for( i = 0; i < length; ++i ){
            theOption.clear();
            optionList[i].split( '=', theOption );
            if( theOption[0] == MString("groups") &&
                                                    theOption.length() > 1 ) {
                if( theOption[1].asInt() > 0 ){
                    groups = true;
                }else{
                    groups = false;
                }
            }
            if( theOption[0] == MString("materials") &&
                                                    theOption.length() > 1 ) {
                if( theOption[1].asInt() > 0 ){
                    materials = true;
                }else{
                    materials = false;
                }
            }
            if( theOption[0] == MString("ptgroups") &&
                                                    theOption.length() > 1 ) {
                if( theOption[1].asInt() > 0 ){
                    ptgroups = true;
                }else{
                    ptgroups = false;
                }
            }
            if( theOption[0] == MString("normals") &&
                                                    theOption.length() > 1 ) {
                if( theOption[1].asInt() > 0 ){
                    normals = true;
                }else{
                    normals = false;
                }
            }
            if( theOption[0] == MString("smoothing") &&
                                                    theOption.length() > 1 ) {
                if( theOption[1].asInt() > 0 ){
                    smoothing = true;
                }else{
                    smoothing = false;
                }
            }
        }
    }

    /* print current linear units used as a comment in the obj file */
    setToLongUnitName(MDistance::uiUnit(), unitName);
    //fprintf( fp, "# This file uses %s as units for non-parametric coordinates.\n\n", unitName.asChar() ); 
    fprintf( fp, "# The units used in this file are %s.\n", unitName.asChar() );

    if( ( mode == MPxFileTranslator::kExportAccessMode ) ||
        ( mode == MPxFileTranslator::kSaveAccessMode ) )
    {
        exportAll();
    }
    else if( mode == MPxFileTranslator::kExportActiveAccessMode )
    {
        exportSelected();
    }
    fclose(fp);

    return MS::kSuccess;
}
//////////////////////////////////////////////////////////////

void ObjTranslator::setToLongUnitName(const MDistance::Unit &unit, MString& unitName)
{
    switch( unit ) 
	{
	case MDistance::kInches:
		/// Inches
		unitName = "inches";
		break;
	case MDistance::kFeet:
		/// Feet
		unitName = "feet";
		break;
	case MDistance::kYards:
		/// Yards
		unitName = "yards";
		break;
	case MDistance::kMiles:
		/// Miles
		unitName = "miles";
		break;
	case MDistance::kMillimeters:
		/// Millimeters
		unitName = "millimeters";
		break;
	case MDistance::kCentimeters:
		/// Centimeters
		unitName = "centimeters";
		break;
	case MDistance::kKilometers:
		/// Kilometers
		unitName = "kilometers";
		break;
	case MDistance::kMeters:
		/// Meters
		unitName = "meters";
		break;
	default:
		break;
	}
}
//////////////////////////////////////////////////////////////

bool ObjTranslator::haveReadMethod () const
{
    return false;
}
//////////////////////////////////////////////////////////////

bool ObjTranslator::haveWriteMethod () const
{
    return true;
}
//////////////////////////////////////////////////////////////

MString ObjTranslator::defaultExtension () const
{
    return "obj";
}
//////////////////////////////////////////////////////////////

MPxFileTranslator::MFileKind ObjTranslator::identifyFile (
                                        const MFileObject& fileName,
                                        const char* buffer,
                                        short size) const
{
	const char * name = fileName.resolvedName().asChar();
    int   nameLength = strlen(name);
    
    if ((nameLength > 4) && !strcasecmp(name+nameLength-4, ".obj"))
        return kCouldBeMyFileType;
    else
        return kNotMyFileType;
}
//////////////////////////////////////////////////////////////

MStatus initializePlugin( MObject obj )
{
    MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

    // Register the translator with the system
    return plugin.registerFileTranslator( "OBJexport", "none",
                                          ObjTranslator::creator,
                                          (char *)objOptionScript,
                                          (char *)objDefaultOptions );                                        
}
//////////////////////////////////////////////////////////////

MStatus uninitializePlugin( MObject obj )
{
        MFnPlugin plugin( obj );
        return plugin.deregisterFileTranslator( "OBJexport" );
}

//////////////////////////////////////////////////////////////

MStatus ObjTranslator::OutputPolygons( 
        MDagPath& mdagPath,
        MObject&  mComponent
)
{
	MStatus stat = MS::kSuccess;
	MSpace::Space space = MSpace::kWorld;
	int i;

	MFnMesh fnMesh( mdagPath, &stat );
	if ( MS::kSuccess != stat) {
		fprintf(stderr,"Failure in MFnMesh initialization.\n");
		return MS::kFailure;
	}

	MItMeshPolygon polyIter( mdagPath, mComponent, &stat );
	if ( MS::kSuccess != stat) {
		fprintf(stderr,"Failure in MItMeshPolygon initialization.\n");
		return MS::kFailure;
	}
	MItMeshVertex vtxIter( mdagPath, mComponent, &stat );
	if ( MS::kSuccess != stat) {
		fprintf(stderr,"Failure in MItMeshVertex initialization.\n");
		return MS::kFailure;
	}

	int objectIdx = -1, length;
	MString mdagPathNodeName = fnMesh.name();
	// Find i such that objectGroupsTablePtr[i] corresponds to the
	// object node pointed to by mdagPath
	length = objectNodeNamesArray.length();
	for( i=0; i<length; i++ ) {
		if( objectNodeNamesArray[i] == mdagPathNodeName ) {
			objectIdx = i;
			break;
		}
	}

    // Write out the vertex table
    //

	for ( ; !vtxIter.isDone(); vtxIter.next() ) {
		MPoint p = vtxIter.position( space );
		if (ptgroups && groups && (objectIdx >= 0)) {
			int compIdx = vtxIter.index();
		    outputSetsAndGroups( mdagPath, compIdx, true, objectIdx );
		}
		// convert from internal units to the current ui units
		p.x = MDistance::internalToUI(p.x);
		p.y = MDistance::internalToUI(p.y);
		p.z = MDistance::internalToUI(p.z);
		fprintf(fp,"v %f %f %f\n",p.x,p.y,p.z);
		v++;
	}

    // Write out the uv table
    //
	MFloatArray uArray, vArray;
	fnMesh.getUVs( uArray, vArray );
    int uvLength = uArray.length();
	for ( int x=0; x<uvLength; x++ ) {
		fprintf(fp,"vt %f %f\n",uArray[x],vArray[x]);
		vt++;
	}

    // Write out the normal table
    //
    if ( normals ) {
	    MFloatVectorArray norms;
	    fnMesh.getNormals( norms, MSpace::kWorld );
        int normsLength = norms.length();
	    for ( int t=0; t<normsLength; t++ ) {
	    	MFloatVector tmpf = norms[t];
	    	fprintf(fp,"vn %f %f %f\n",tmpf[0],tmpf[1],tmpf[2]);
	    	vn++;
	    }
    }

    // For each polygon, write out: 
    //    s  smoothing_group
    //    sets/groups the polygon belongs to 
    //    f  vertex_index/uvIndex/normalIndex
    //
    int lastSmoothingGroup = INITIALIZE_SMOOTHING;

	for ( ; !polyIter.isDone(); polyIter.next() )
	{
        // Write out the smoothing group that this polygon belongs to
        // We only write out the smoothing group if it is different
        // from the last polygon.
        //
        if ( smoothing ) {
           	int compIdx = polyIter.index();
            int smoothingGroup = polySmoothingGroups[ compIdx ];
            
            if ( lastSmoothingGroup != smoothingGroup ) {
                if ( NO_SMOOTHING_GROUP == smoothingGroup ) {
                    fprintf(fp,"s off\n");
                }
                else {
                    fprintf(fp,"s %d\n", smoothingGroup );
                }
                lastSmoothingGroup = smoothingGroup;
            }
        }
        
        // Write out all the sets that this polygon belongs to
        //
		if ((groups || materials) && (objectIdx >= 0)) {
			int compIdx = polyIter.index();
			outputSetsAndGroups( mdagPath, compIdx, false, objectIdx );
		}
                
        // Write out vertex/uv/normal index information
        //
		fprintf(fp,"f");
        int polyVertexCount = polyIter.polygonVertexCount();
		for ( int vtx=0; vtx<polyVertexCount; vtx++ ) {
			fprintf(fp," %d", polyIter.vertexIndex( vtx ) +1 +voff);

            bool noUV = true;
			if ( fnMesh.numUVs() > 0 ) {
    			int uvIndex;
                // If the call to getUVIndex fails then there is no
                // mapping information for this polyon so we don't write
                // anything
                //
    			if ( polyIter.getUVIndex(vtx,uvIndex) ) {
    			    fprintf(fp,"/%d",uvIndex+1 +vtoff);
                    noUV = false;
                }
			}
            
			if ( (normals) && (fnMesh.numNormals() > 0) ) {
                if ( noUV ) {
                    // If there are no UVs then our polygon is written
                    // in the form vertex//normal
                    //
                    fprintf(fp,"/");
                }
                fprintf(fp,"/%d",polyIter.normalIndex( vtx ) +1 +vnoff);
            }
		}
		fprintf(fp,"\n");
		// Do not do this ... it is not neccessary and is deadly on windows
		// see MAYA-17370 - Ming
		//
//		fflush(fp);
	}
	return stat;
}
//////////////////////////////////////////////////////////////

void ObjTranslator::outputSetsAndGroups( 
    MDagPath & mdagPath, 
	int cid,
	bool isVertexIterator,
	int objectIdx
	
)
{
    MStatus stat;
	
    int i, length;
	MIntArray * currentSets = new MIntArray;
	MIntArray * currentMaterials = new MIntArray;
	MStringArray gArray, mArray;


	if (groups || materials) {

		for ( i=0; i<numSets; i++ )
		{
			if ( lookup(mdagPath,i,cid,isVertexIterator) ) {
			
				MFnSet fnSet( (*sets)[i] );
				if ( MFnSet::kRenderableOnly == fnSet.restriction(&stat) ) {
					currentMaterials->append( i );
					mArray.append( fnSet.name() );
				}
				else {
					currentSets->append( i );
					gArray.append( fnSet.name() );
				}
			}
		}

		if( !isVertexIterator ) {
			// export group nodes (transform DAG nodes) in Maya that
			// the current object is a
			// child/grandchild/grandgrandchild/... of
			bool *objectGroupTable = objectGroupsTablePtr[objectIdx];
			length = transformNodeNameArray.length();
			for( i=0; i<length; i++ ) {
				if( objectGroupTable[i] ) {
					currentSets->append( numSets + i );
					gArray.append(transformNodeNameArray[i]);
				}
			}
		}

		// prevent grouping incoherence, use tav default group schema.
		//
		if (0 == currentSets->length())
		{
			currentSets->append( 0 );
			gArray.append( "default" );
		}
		
					
		// Test for equivalent sets
		//
		bool setsEqual = false;
		if ( (lastSets != NULL) && 
			  (lastSets->length() == currentSets->length())
		) {
			setsEqual = true;
			length = lastSets->length();
			for ( i=0; i<length; i++ )
			{
				if ( (*lastSets)[i] != (*currentSets)[i] ) {
					setsEqual = false;
					break;
				}
			}	
		}

		if ( !setsEqual ) {
			if ( lastSets != NULL )
				delete lastSets;
		
			lastSets = currentSets;		
		
			if (groups) {
				int gLength = gArray.length();
			    if ( gLength > 0  ) {
			        fprintf(fp,"g");
			        for ( i=0; i<gLength; i++ ) {
			            fprintf(fp," %s",gArray[i].asChar());
			        }
			        fprintf(fp,"\n");
			    }
			}
		}
		else
		{
			delete currentSets;
		}

		


		// Test for equivalent materials
		//
		bool materialsEqual = false;
		if ( (lastMaterials != NULL) && 
			  (lastMaterials->length() == currentMaterials->length())
		) {
			materialsEqual = true;
			length = lastMaterials->length();
			for ( i=0; i<length; i++ )
			{
				if ( (*lastMaterials)[i] != (*currentMaterials)[i] ) {
					materialsEqual = false;
					break;
				}
			}			
		}

		if ( !materialsEqual ) {
			if ( lastMaterials != NULL )
				delete lastMaterials;
	
			lastMaterials = currentMaterials;
	
			if (materials) {


				int mLength = mArray.length();

				if ( mLength > 0  ) {
			    	fprintf(fp,"usemtl");
			    	for ( i=0; i<mLength; i++ ) {
			        	fprintf(fp," %s",mArray[i].asChar());
			    	}
			    	fprintf(fp,"\n");
				}
			}
		}
		else
		{
			delete currentMaterials;
		}
	}	
}

//////////////////////////////////////////////////////////////

void ObjTranslator::initializeSetsAndLookupTables( bool exportAll )
//
// Description :
//    Creates a list of all sets in Maya, a list of mesh objects,
//    and polygon/vertex lookup tables that will be used to
//    determine which sets are referenced by the poly components.
//
{
	int i=0,j=0, length;
	MStatus stat;
	
	// Initialize class data.
	// Note: we cannot do this in the constructor as it
	// only gets called upon registry of the plug-in.
	//
	numSets = 0;
	sets = NULL;
	lastSets = NULL;
	lastMaterials = NULL;
	objectId = 0;
	objectCount = 0;
	polygonTable = NULL;
	vertexTable = NULL;
	polygonTablePtr = NULL;
	vertexTablePtr = NULL;
	objectGroupsTablePtr = NULL;
	objectNodeNamesArray.clear();
	transformNodeNameArray.clear();

	//////////////////////////////////////////////////////////////////
	//
	// Find all sets in Maya and store the ones we care about in
	// the 'sets' array. Also make note of the number of sets.
	//
	//////////////////////////////////////////////////////////////////
	
	// Get all of the sets in maya and put them into
	// a selection list
	// 
	MStringArray result;
	MGlobal::executeCommand( "ls -sets", result );
	MSelectionList * setList = new MSelectionList;
	length = result.length();
	for ( i=0; i<length; i++ )
	{	
		setList->add( result[i] );
	}
	
	// Extract each set as an MObject and add them to the
	// sets array.
	// We may be excluding groups, matierials, or ptGroups
	// in which case we can ignore those sets. 
	//
	MObject mset;
	sets = new MObjectArray();
	length = setList->length();
	for ( i=0; i<length; i++ )
	{
		setList->getDependNode( i, mset );
		
		MFnSet fnSet( mset, &stat );
		if ( stat ) {
			if ( MFnSet::kRenderableOnly == fnSet.restriction(&stat) ) {
				if ( materials ) {
					sets->append( mset );
				}
			} 
			else {
				if ( groups ) {
					sets->append( mset );
				}
			}
		}	
	}
	delete setList;
	
	numSets = sets->length();
			
	//////////////////////////////////////////////////////////////////
	//
	// Do a dag-iteration and for every mesh found, create facet and
	// vertex look-up tables. These tables will keep track of which
	// sets each component belongs to.
	//
	// If exportAll is false then iterate over the activeSelection 
	// list instead of the entire DAG.
	//
	// These arrays have a corrisponding entry in the name
	// stringArray.
	//
	//////////////////////////////////////////////////////////////////
	MIntArray vertexCounts;
	MIntArray polygonCounts;	
			
	if ( exportAll ) {
		MItDag dagIterator( MItDag::kBreadthFirst, MFn::kInvalid, &stat);

    	if ( MS::kSuccess != stat) {
    	    fprintf(stderr,"Failure in DAG iterator setup.\n");
    	    return;
    	}
		
		objectNames = new MStringArray;
		
    	for ( ; !dagIterator.isDone(); dagIterator.next() ) 
		{
    	    MDagPath dagPath;
    	    stat = dagIterator.getPath( dagPath );

			if ( stat ) 
			{
				// skip over intermediate objects
				//
				MFnDagNode dagNode( dagPath, &stat );
				if (dagNode.isIntermediateObject()) 
				{
					continue;
				}

				if (( dagPath.hasFn(MFn::kMesh)) &&
					( dagPath.hasFn(MFn::kTransform)))
				{
					// We want only the shape, 
					// not the transform-extended-to-shape.
					continue;
				}
				else if ( dagPath.hasFn(MFn::kMesh))
				{
					// We have a mesh so create a vertex and polygon table
					// for this object.
					//
					MFnMesh fnMesh( dagPath );
					int vtxCount = fnMesh.numVertices();
					int polygonCount = fnMesh.numPolygons();
					// we do not need this call anymore, we have the shape.
					// dagPath.extendToShape();
					MString name = dagPath.fullPathName();
					objectNames->append( name );
					objectNodeNamesArray.append( fnMesh.name() );

					vertexCounts.append( vtxCount );
					polygonCounts.append( polygonCount );

					objectCount++;
				}
			}
		}	
	}
	else 
	{
		MSelectionList slist;
    	MGlobal::getActiveSelectionList( slist );
    	MItSelectionList iter( slist );
		MStatus status;

		objectNames = new MStringArray;

		// We will need to interate over a selected node's heirarchy
		// in the case where shapes are grouped, and the group is selected.
		MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &status);

    	for ( ; !iter.isDone(); iter.next() ) 
		{
			MDagPath objectPath;
			stat = iter.getDagPath( objectPath );

			// reset iterator's root node to be the selected node.
			status = dagIterator.reset (objectPath.node(), 
										MItDag::kDepthFirst, MFn::kInvalid );

			// DAG iteration beginning at at selected node
			for ( ; !dagIterator.isDone(); dagIterator.next() )
			{
				MDagPath dagPath;
				MObject  component = MObject::kNullObj;
				status = dagIterator.getPath(dagPath);

				if (!status) {
					fprintf(stderr,"Failure getting DAG path.\n");
					freeLookupTables();
					return ;
				}

                // skip over intermediate objects
                //
                MFnDagNode dagNode( dagPath, &stat );
                if (dagNode.isIntermediateObject()) 
                {
                    continue;
                }


				if (( dagPath.hasFn(MFn::kMesh)) &&
					( dagPath.hasFn(MFn::kTransform)))
				{
					// We want only the shape, 
					// not the transform-extended-to-shape.
					continue;
				}
				else if ( dagPath.hasFn(MFn::kMesh))
				{
					// We have a mesh so create a vertex and polygon table
					// for this object.
					//
					MFnMesh fnMesh( dagPath );
					int vtxCount = fnMesh.numVertices();
					int polygonCount = fnMesh.numPolygons();

					// we do not need this call anymore, we have the shape.
					// dagPath.extendToShape();
					MString name = dagPath.fullPathName();
					objectNames->append( name );
					objectNodeNamesArray.append( fnMesh.name() );
									
					vertexCounts.append( vtxCount );
					polygonCounts.append( polygonCount );

					objectCount++;	
				}
    		}
		}
	}

	// Now we know how many objects we are dealing with 
	// and we have counts of the vertices/polygons for each
	// object so create the maya group look-up table.
	//
	if( objectCount > 0 ) {

		// To export Maya groups we traverse the hierarchy starting at
		// each objectNodeNamesArray[i] going towards the root collecting transform
		// nodes as we go.
		length = objectNodeNamesArray.length();
		for( i=0; i<length; i++ ) {
			MIntArray transformNodeNameIndicesArray;
			recFindTransformDAGNodes( objectNodeNamesArray[i], transformNodeNameIndicesArray );
		}

		if( transformNodeNameArray.length() > 0 ) {
			objectGroupsTablePtr = (bool**) malloc( sizeof(bool*)*objectCount );
			length = transformNodeNameArray.length();
			for ( i=0; i<objectCount; i++ )
			{
				objectGroupsTablePtr[i] =
					(bool*)calloc( length, sizeof(bool) );	
				
				if ( objectGroupsTablePtr[i] == NULL ) {
					cerr << "Error: calloc returned NULL (objectGroupsTablePtr)\n";
					return;
				}
			}
		}
	}

	// Create the vertex/polygon look-up tables.
	//
	if ( objectCount > 0 ) {
		
		vertexTablePtr = (bool**) malloc( sizeof(bool*)*objectCount );
		polygonTablePtr = (bool**) malloc( sizeof(bool*)*objectCount );
	
		for ( i=0; i<objectCount; i++ )
		{
			vertexTablePtr[i] =
				 (bool*)calloc( vertexCounts[i]*numSets, sizeof(bool) );	

			if ( vertexTablePtr[i] == NULL ) {
				cerr << "Error: calloc returned NULL (vertexTable)\n";
				return;
			}
	
			polygonTablePtr[i] =
				 (bool*)calloc( polygonCounts[i]*numSets, sizeof(bool) );
			if ( polygonTablePtr[i] == NULL ) {
				cerr << "Error: calloc returned NULL (polygonTable)\n";
				return;
			}
		}	
	}

	// If we found no meshes then return
	//	
	if ( objectCount == 0 ) {
		return;
	}
	
	//////////////////////////////////////////////////////////////////
	//
	// Go through all of the set members (flattened lists) and mark
	// in the lookup-tables, the sets that each mesh component belongs
	// to.
	//
	//
	//////////////////////////////////////////////////////////////////
	bool flattenedList = true;
	MDagPath object;
	MObject component;
	MSelectionList memberList;
	
	
	for ( i=0; i<numSets; i++ )
	{
		MFnSet fnSet( (*sets)[i] );		
		memberList.clear();
		stat = fnSet.getMembers( memberList, flattenedList );

		if (MS::kSuccess != stat) {
			fprintf(stderr,"Error in fnSet.getMembers()!\n");
		}

		int m, numMembers;
		numMembers = memberList.length();
		for ( m=0; m<numMembers; m++ )
		{
			if ( memberList.getDagPath(m,object,component) ) {

				if ( (!component.isNull()) && (object.apiType() == MFn::kMesh) )
				{
					if (component.apiType() == MFn::kMeshVertComponent) {
						MItMeshVertex viter( object, component );	
						for ( ; !viter.isDone(); viter.next() )
						{
							int compIdx = viter.index();
							MString name = object.fullPathName();
							
							// Figure out which object vertexTable
							// to get.
							//

							int o, numObjectNames;
							numObjectNames = objectNames->length();
							for ( o=0; o<numObjectNames; o++ ) {
								if ( (*objectNames)[o] == name ) {
									// Mark set i as true in the table
									//		
									vertexTable = vertexTablePtr[o];
									*(vertexTable + numSets*compIdx + i) = true;
									break;
								}
							}
						}
					}
					else if (component.apiType() == MFn::kMeshPolygonComponent) 
					{
						MItMeshPolygon piter( object, component );
						for ( ; !piter.isDone(); piter.next() )
						{
							int compIdx = piter.index();
							MString name = object.fullPathName();
							
							// Figure out which object polygonTable
							// to get.
							//							
							int o, numObjectNames;
							numObjectNames = objectNames->length();
							for ( o=0; o<numObjectNames; o++ ) {
								if ( (*objectNames)[o] == name ) {
									
									// Mark set i as true in the table
									//

// Check for bad components in the set
//									
if ( compIdx >= polygonCounts[o] ) {
	cerr << "Error: component in set >= numPolygons, skipping!\n";
	cerr << "  Component index    = " << compIdx << endl;
	cerr << "  Number of polygons = " << polygonCounts[o] << endl;
	break;
}
									
									polygonTable = polygonTablePtr[o];
									*(polygonTable + numSets*compIdx + i) = true;
									break;
								}
							}	
						}
					}										
				}
				else { 

				// There are no components, therefore we can mark
				// all polygons as members of the given set.
				//

				if (object.hasFn(MFn::kMesh)) {

					MFnMesh fnMesh( object, &stat );
					if ( MS::kSuccess != stat) {
						fprintf(stderr,"Failure in MFnMesh initialization.\n");
						return;
					}

					// We are going to iterate over all the polygons.
					//
					MItMeshPolygon piter( object, MObject::kNullObj, &stat );
					if ( MS::kSuccess != stat) {
						fprintf(stderr,
								"Failure in MItMeshPolygon initialization.\n");
						return;
					}
					for ( ; !piter.isDone(); piter.next() )
					{
						int compIdx = piter.index();
						MString name = object.fullPathName();

						// Figure out which object polygonTable to get.
						//
						int o, numObjectNames;
						numObjectNames = objectNames->length();
						for ( o=0; o<numObjectNames; o++ ) {
							if ( (*objectNames)[o] == name ) {

// Check for bad components in the set
//
if ( compIdx >= polygonCounts[o] ) {
	cerr << "Error: component in set >= numPolygons, skipping!\n";
	cerr << "  Component index    = " << compIdx << endl;
	cerr << "  Number of polygons = " << polygonCounts[o] << endl;
	break;
}
								// Mark set i as true in the table
								//
								polygonTable = polygonTablePtr[o];
								*(polygonTable + numSets*compIdx + i) = true;
								break;
							}
						}
					} // end of piter.next() loop
				} // end of condition if (object.hasFn(MFn::kMesh))
				} // end of else condifion if (!component.isNull()) 
			} // end of memberList.getDagPath(m,object,component)
		} // end of memberList loop
	} // end of for-loop for sets

	// Go through all of the group members and mark in the
	// lookup-table, the group that each shape belongs to.
	length = objectNodeNamesArray.length();
	for( i=0; i<length; i++ ) {
		MIntArray groupTableIndicesArray;
		bool *objectGroupTable = objectGroupsTablePtr[i];
		int length2;
		recFindTransformDAGNodes( objectNodeNamesArray[i], groupTableIndicesArray );
		length2 = groupTableIndicesArray.length();
		for( j=0; j<length2; j++ ) {
			int groupIdx = groupTableIndicesArray[j];
			objectGroupTable[groupIdx] = true;
		}
	}
}

//////////////////////////////////////////////////////////////

void ObjTranslator::freeLookupTables()
//
// Frees up all tables and arrays allocated by this plug-in.
//
{
	for ( int i=0; i<objectCount; i++ ) {
		if ( vertexTablePtr[i] != NULL ) {
			free( vertexTablePtr[i] );
		}
		if ( polygonTablePtr[i] != NULL ) {
			free( polygonTablePtr[i] );
		}
	}	

	if( objectGroupsTablePtr != NULL ) {
		for ( int i=0; i<objectCount; i++ ) {
			if ( objectGroupsTablePtr[i] != NULL ) {
				free( objectGroupsTablePtr[i] );
			}
		}
		free( objectGroupsTablePtr );
		objectGroupsTablePtr = NULL;
	}
	
	if ( vertexTablePtr != NULL ) {
		free( vertexTablePtr );
		vertexTablePtr = NULL;
	}
	if ( polygonTablePtr != NULL ) {
		free( polygonTablePtr );
		polygonTablePtr = NULL;
	}

	if ( lastSets != NULL ) {
		delete lastSets;
		lastSets = NULL;
	}
	
	if ( lastMaterials != NULL ) {
		delete lastMaterials;
		lastMaterials = NULL;
	}
	
	if ( sets != NULL ) {
		delete sets;
		sets = NULL;
	}
	
	if ( objectNames != NULL ) {
		delete objectNames;
		objectNames = NULL;
	}		
}

//////////////////////////////////////////////////////////////

bool ObjTranslator::lookup( MDagPath& dagPath, 
							int setIndex,
							int compIdx,
							bool isVtxIter )
{

	if (isVtxIter) {
		vertexTable = vertexTablePtr[objectId];
		bool ret = *(vertexTable + numSets*compIdx + setIndex);
		return ret;
	}
	else  {				
		polygonTable = polygonTablePtr[objectId];
		bool ret = *(polygonTable + numSets*compIdx + setIndex);			
		return ret;
	}
}	

//////////////////////////////////////////////////////////////

void ObjTranslator::buildEdgeTable( MDagPath& mesh )
{
    if ( !smoothing )
        return;
    
    // Create our edge lookup table and initialize all entries to NULL
    //
    MFnMesh fnMesh( mesh );
    edgeTableSize = fnMesh.numVertices();
    edgeTable = (EdgeInfoPtr*) calloc( edgeTableSize, sizeof(EdgeInfoPtr) );

    // Add entries, for each edge, to the lookup table
    //
    MItMeshEdge eIt( mesh );
    for ( ; !eIt.isDone(); eIt.next() )
    {
        bool smooth = eIt.isSmooth();
        addEdgeInfo( eIt.index(0), eIt.index(1), smooth );
    }

    // Fill in referenced polygons
    //
    MItMeshPolygon pIt( mesh );
    for ( ; !pIt.isDone(); pIt.next() )
    {
        int pvc = pIt.polygonVertexCount();
        for ( int v=0; v<pvc; v++ )
        {
            int a = pIt.vertexIndex( v );
            int b = pIt.vertexIndex( v==(pvc-1) ? 0 : v+1 );

            EdgeInfoPtr elem = findEdgeInfo( a, b );
            if ( NULL != elem ) {
                int edgeId = pIt.index();
                
                if ( INVALID_ID == elem->polyIds[0] ) {
                    elem->polyIds[0] = edgeId;
                }
                else {
                    elem->polyIds[1] = edgeId;
                }                
                    
            }
        }
    }

    // Now create a polyId->smoothingGroup table
    //   
    int numPolygons = fnMesh.numPolygons();
    polySmoothingGroups = (int*)malloc( sizeof(int) *  numPolygons );
    for ( int i=0; i< numPolygons; i++ ) {
        polySmoothingGroups[i] = NO_SMOOTHING_GROUP;
    }    
    
    // Now call the smoothingAlgorithm to fill in the polySmoothingGroups
    // table.
    // Note: we have to traverse ALL polygons to handle the case
    // of disjoint polygons.
    //
    nextSmoothingGroup = 1;
    currSmoothingGroup = 1;
    for ( int pid=0; pid<numPolygons; pid++ ) {
        newSmoothingGroup = true;
        // Check polygon has not already been visited
        if ( NO_SMOOTHING_GROUP == polySmoothingGroups[pid] ) {
           if ( !smoothingAlgorithm(pid,fnMesh) ) {
               // No smooth edges for this polygon so we set
               // the smoothing group to NO_SMOOTHING_GROUP (off)
               polySmoothingGroups[pid] = NO_SMOOTHING_GROUP;
           }
        }
    }
}

//////////////////////////////////////////////////////////////

bool ObjTranslator::smoothingAlgorithm( int polyId, MFnMesh& fnMesh )
{
    MIntArray vertexList;
    fnMesh.getPolygonVertices( polyId, vertexList );
    int vcount = vertexList.length();
    bool smoothEdgeFound = false;
    
    for ( int vid=0; vid<vcount;vid++ ) {
        int a = vertexList[vid];
        int b = vertexList[ vid==(vcount-1) ? 0 : vid+1 ];
        
        EdgeInfoPtr elem = findEdgeInfo( a, b );
        if ( NULL != elem ) {
            // NOTE: We assume there are at most 2 polygons per edge
            //       halfEdge polygons get a smoothing group of
            //       NO_SMOOTHING_GROUP which is equivalent to "s off"
            //
            if ( NO_SMOOTHING_GROUP != elem->polyIds[1] ) { // Edge not a border
                
                // We are starting a new smoothing group
                //                
                if ( newSmoothingGroup ) {
                    currSmoothingGroup = nextSmoothingGroup++;
                    newSmoothingGroup = false;
                    
                    // This is a SEED (starting) polygon and so we always
                    // give it the new smoothing group id.
                    // Even if all edges are hard this must be done so
                    // that we know we have visited the polygon.
                    //
                    polySmoothingGroups[polyId] = currSmoothingGroup;
                }
                
                // If we have a smooth edge then this poly must be a member
                // of the current smoothing group.
                //
                if ( elem->smooth ) {
                    polySmoothingGroups[polyId] = currSmoothingGroup;
                    smoothEdgeFound = true;
                }
                else { // Hard edge so ignore this polygon
                    continue;
                }
                
                // Find the adjacent poly id
                //
                int adjPoly = elem->polyIds[0];
                if ( adjPoly == polyId ) {
                    adjPoly = elem->polyIds[1];
                }                             
                
                // If we are this far then adjacent poly belongs in this
                // smoothing group.
                // If the adjacent polygon's smoothing group is not
                // NO_SMOOTHING_GROUP then it has already been visited
                // so we ignore it.
                //
                if ( NO_SMOOTHING_GROUP == polySmoothingGroups[adjPoly] ) {
                    smoothingAlgorithm( adjPoly, fnMesh );
                }
                else if ( polySmoothingGroups[adjPoly] != currSmoothingGroup ) {
                    cerr << "Warning: smoothing group problem at polyon ";
                    cerr << adjPoly << endl;
                }
            }
        }
    }
    return smoothEdgeFound;
}

//////////////////////////////////////////////////////////////

void ObjTranslator::addEdgeInfo( int v1, int v2, bool smooth )
//
// Adds a new edge info element to the vertex table.
//
{
    EdgeInfoPtr element = NULL;

    if ( NULL == edgeTable[v1] ) {
        edgeTable[v1] = (EdgeInfoPtr)malloc( sizeof(struct EdgeInfo) );
        element = edgeTable[v1];
    }
    else {
        element = edgeTable[v1];
        while ( NULL != element->next ) {
            element = element->next;
        }
        element->next = (EdgeInfoPtr)malloc( sizeof(struct EdgeInfo) );
        element = element->next;
    }

    // Setup data for new edge
    //
    element->vertId     = v2;
    element->smooth     = smooth;
    element->next       = NULL;
   
    // Initialize array of id's of polygons that reference this edge.
    // There are at most 2 polygons per edge.
    //
    element->polyIds[0] = INVALID_ID;
    element->polyIds[1] = INVALID_ID;
}

//////////////////////////////////////////////////////////////

EdgeInfoPtr ObjTranslator::findEdgeInfo( int v1, int v2 )
//
// Finds the info for the specified edge.
//
{
    EdgeInfoPtr element = NULL;
    element = edgeTable[v1];

    while ( NULL != element ) {
        if ( v2 == element->vertId ) {
            return element;
        }
        element = element->next;
    }
    
    if ( element == NULL ) {
        element = edgeTable[v2];

        while ( NULL != element ) {
            if ( v1 == element->vertId ) {
                return element;
            }
            element = element->next;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////

void ObjTranslator::destroyEdgeTable()
//
// Free up all of the memory used by the edgeTable.
//
{
    if ( !smoothing )
        return;
    
    EdgeInfoPtr element = NULL;
    EdgeInfoPtr tmp = NULL;

    for ( int v=0; v<edgeTableSize; v++ )
    {
        element = edgeTable[v];
        while ( NULL != element )
        {
            tmp = element;
            element = element->next;
            free( tmp );
        }
    }

    if ( NULL != edgeTable ) {
        free( edgeTable );
        edgeTable = NULL;
    }
    
    if ( NULL != polySmoothingGroups ) {
        free( polySmoothingGroups );
        polySmoothingGroups = NULL;
    }
}

//////////////////////////////////////////////////////////////

MStatus ObjTranslator::exportSelected( )
{
	MStatus status;
	MString filename;

	initializeSetsAndLookupTables( false );

	// Create an iterator for the active selection list
	//
	MSelectionList slist;
	MGlobal::getActiveSelectionList( slist );
	MItSelectionList iter( slist );

	if (iter.isDone()) {
	    fprintf(stderr,"Error: Nothing is selected.\n");
	    return MS::kFailure;
	}

    // We will need to interate over a selected node's heirarchy 
    // in the case where shapes are grouped, and the group is selected.
    MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &status);

	// reset counters
	v = vt = vn = 0;
	voff = vtoff = vnoff = 0;

	// Selection list loop
	for ( ; !iter.isDone(); iter.next() )
	{	 
		MDagPath objectPath;
		// get the selected node
		status = iter.getDagPath( objectPath);

		// reset iterator's root node to be the selected node.
		status = dagIterator.reset (objectPath.node(), 
									MItDag::kDepthFirst, MFn::kInvalid );	

		// DAG iteration beginning at at selected node
		for ( ; !dagIterator.isDone(); dagIterator.next() )
		{
			MDagPath dagPath;
			MObject  component = MObject::kNullObj;
			status = dagIterator.getPath(dagPath);

			if (!status) {
				fprintf(stderr,"Failure getting DAG path.\n");
				freeLookupTables();
				return MS::kFailure;
			}

			if (status ) 
			{
                // skip over intermediate objects
                //
                MFnDagNode dagNode( dagPath, &status );
                if (dagNode.isIntermediateObject()) 
                {
                    continue;
                }

				if (dagPath.hasFn(MFn::kNurbsSurface))
				{
					status = MS::kSuccess;
					fprintf(stderr,"Warning: skipping Nurbs Surface.\n");
				}
				else if ((  dagPath.hasFn(MFn::kMesh)) &&
						 (  dagPath.hasFn(MFn::kTransform)))
				{
					// We want only the shape, 
					// not the transform-extended-to-shape.
					continue;
				}
				else if (  dagPath.hasFn(MFn::kMesh))
				{
					// Build a lookup table so we can determine which 
					// polygons belong to a particular edge as well as
					// smoothing information
					//
					buildEdgeTable( dagPath );
					
					status = OutputPolygons(dagPath, component);
					objectId++;
					if (status != MS::kSuccess) {
						fprintf(stderr, "Error: exporting geom failed, check your selection.\n");
						freeLookupTables();
						destroyEdgeTable(); // Free up the edge table				
						return MS::kFailure;
					}
					destroyEdgeTable(); // Free up the edge table				
				}
				voff = v;
				vtoff = vt;
				vnoff = vn;
			}
		}
	}
	
	freeLookupTables();
	
	return status;
}

//////////////////////////////////////////////////////////////

MStatus ObjTranslator::exportAll( )
{
	MStatus status = MS::kSuccess;

	initializeSetsAndLookupTables( true );

	MItDag dagIterator( MItDag::kBreadthFirst, MFn::kInvalid, &status);

	if ( MS::kSuccess != status) {
	    fprintf(stderr,"Failure in DAG iterator setup.\n");
	    return MS::kFailure;
	}
	// reset counters
	v = vt = vn = 0;
	voff = vtoff = vnoff = 0;

	for ( ; !dagIterator.isDone(); dagIterator.next() )
	{
		MDagPath dagPath;
		MObject  component = MObject::kNullObj;
		status = dagIterator.getPath(dagPath);

		if (!status) {
			fprintf(stderr,"Failure getting DAG path.\n");
			freeLookupTables();
			return MS::kFailure;
		}

		// skip over intermediate objects
		//
		MFnDagNode dagNode( dagPath, &status );
		if (dagNode.isIntermediateObject()) 
		{
			continue;
		}

		if ((  dagPath.hasFn(MFn::kNurbsSurface)) &&
			(  dagPath.hasFn(MFn::kTransform)))
		{
			status = MS::kSuccess;
			fprintf(stderr,"Warning: skipping Nurbs Surface.\n");
		}
		else if ((  dagPath.hasFn(MFn::kMesh)) &&
				 (  dagPath.hasFn(MFn::kTransform)))
		{
			// We want only the shape, 
			// not the transform-extended-to-shape.
			continue;
		}
		else if (  dagPath.hasFn(MFn::kMesh))
		{
            // Build a lookup table so we can determine which 
            // polygons belong to a particular edge as well as
            // smoothing information
            //
            buildEdgeTable( dagPath );
			
            // Now output the polygon information
            //
            status = OutputPolygons(dagPath, component);
			objectId++;
			if (status != MS::kSuccess) {
				fprintf(stderr,"Error: exporting geom failed.\n");
				freeLookupTables();                
                destroyEdgeTable(); // Free up the edge table				
                return MS::kFailure;
            }
            destroyEdgeTable(); // Free up the edge table
		}
		voff = v;
		vtoff = vt;
		vnoff = vn;
	}

	freeLookupTables();

	return status;
}

//////////////////////////////////////////////////////////////

void ObjTranslator::recFindTransformDAGNodes( MString& nodeName, MIntArray& transformNodeIndicesArray )
{
	// To handle Maya groups we traverse the hierarchy starting at
	// each objectNames[i] going towards the root collecting transform
	// nodes as we go.
	MStringArray result;
	MString cmdStr = "listRelatives -ap " + nodeName;
	MGlobal::executeCommand( cmdStr, result );
	
	if( result.length() == 0 )
		// nodeName must be at the root of the DAG.  Stop recursing
		return;

	for( unsigned int j=0; j<result.length(); j++ ) {
		// check if the node result[i] is of type transform
		MStringArray result2;
		MGlobal::executeCommand( "nodeType " + result[j], result2 );
		
		if( result2.length() == 1 && result2[0] == "transform" ) {
			// check if result[j] is already in result[j]
			bool found=false;
			unsigned int i;
			for( i=0; i<transformNodeNameArray.length(); i++) {
				if( transformNodeNameArray[i] == result[j] ) {
					found = true;
					break;
				}
			}

			if( !found ) {
				transformNodeIndicesArray.append(transformNodeNameArray.length());
				transformNodeNameArray.append(result[j]);
			}
			else {
				transformNodeIndicesArray.append(i);
			}
			recFindTransformDAGNodes(result[j], transformNodeIndicesArray);
		}
	}
}
