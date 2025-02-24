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

#include <stdio.h>

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MConditionMessage.h>
#include <maya/MFnPlugin.h>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
// 
// Registers condition callbacks.
//	
// The conditionTest plug-in is a simple plug-in that displays which "conditions" are being changed in Maya.
// A condition in Maya is something of interest to the Maya internals or plug-ins that has a true or false value.
// For example, "SomethingSelected" and "playingBack" are two available conditions.
// These conditions can be tracked at the MEL level with the -conditionTrue, -conditionFalse, or -conditionChanged flags
// to the "scriptJob" command.
//
// The plug-in adds a "conditionTest" command that lets you view which conditions are available, track specific conditions,
// and view which conditions are being tracked. The basic command syntax is:
// conditionTest [-m on|off] [conditionName ...]
//
// The "conditionName" arguments must be the names of conditions. If no names are specified, the command will operate
// on all available conditions.
// 
// If you use the -m flag, you can specify whether the plug-in must track messages for the specified conditions or not.
// If you specify the -m flag without any condition names, it will turn on or turn off tracking for all conditions.
// 
// Example:
// mel: conditionTest -m 1 SomethingSelected
// Condition Name State Msgs On
// -------------------- ----- -------
// SomethingSelected false yes
//
// After this, the plug-in will display the following line each time the selection list becomes empty or not empty:
// "condition SomethingSelected changed to true"
// or
// "condition SomethingSelected changed to false"
// 
// You can disable condition tracking using the command: conditionTest -m off; 
//
////////////////////////////////////////////////////////////////////////

// Message flag
//
#define kMessageFlag		"m"
#define kMessageFlagLong	"message"

///////////////////////////////////////////////////
//
// Declarations
//
///////////////////////////////////////////////////

// Callback function for messages
//
static void conditionChangedCB(bool state, void * data);

// Array of callback ids.
//
typedef MCallbackId* MCallbackIdPtr;
static MCallbackIdPtr callbackId = NULL;

// Array of condition names.
//
static MStringArray conditionNames;

///////////////////////////////////////////////////
//
// Command class declaration
//
///////////////////////////////////////////////////
class conditionTest : public MPxCommand
{
public:
	conditionTest();
	                ~conditionTest() override; 

	MStatus                 doIt( const MArgList& args ) override;

	static MSyntax			newSyntax();
	static void*			creator();

private:
	MStatus                 parseArgs( const MArgList& args );

	bool					addMessage;
	bool					delMessage;

	MStringArray			conditions;
};


///////////////////////////////////////////////////
//
// Command class implementation
//
///////////////////////////////////////////////////

// Constructor
//
conditionTest::conditionTest()
:	addMessage(false)
,	delMessage(false)
{
	conditions.clear();
}

// Destructor
//
conditionTest::~conditionTest()
{
	// Do nothing
}

// creator
//
void* conditionTest::creator()
{
	return (void *) (new conditionTest);
}

// newSyntax
//
MSyntax conditionTest::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag(kMessageFlag, kMessageFlagLong, MSyntax::kBoolean);
	syntax.setObjectType(MSyntax::kStringObjects);

	return syntax;
}

// parseArgs
//
MStatus conditionTest::parseArgs(const MArgList& args)
{
	MStatus			status;
	MArgDatabase	argData(syntax(), args);

	if (argData.isFlagSet(kMessageFlag))
	{
		bool flag;

		status = argData.getFlagArgument(kMessageFlag, 0, flag);
		if (!status)
		{
			status.perror("could not parse message flag");
			return status;
		}

		if (flag)
		{
			addMessage = true;
		}
		else
		{
			delMessage = true;
		}
	}

	status = argData.getObjects(conditions);
	if (!status)
	{
		status.perror("could not parse condition names");
	}

	// If there are no conditions specified, operate on all of them
	//
	if (conditions.length() == 0)
	{
		// conditionNames is set in initializePlugin to all the
		// currently available condition names.
		//
		conditions = conditionNames;
	}

	return status;
}

// doIt
//
MStatus conditionTest::doIt(const MArgList& args)
{
	MStatus status;

	status = parseArgs(args);
	if (!status)
	{
		return status;
	}

	// Allocate an array of indices.  conditions[n] is a user provided
	// condition name.  Look it up in the static conditionNames array
	// and set indices[n] to the index of the entry in conditionNames.
	//
	// This maps the user specified conditions to the global conditions
	// so we can track callback adds and removes globally.
	//
	int * indices = new int [conditions.length()];

	int i, j;

	for (i = 0; i < (int)conditions.length(); ++i)
	{
		// Initialize the entry to "not found".
		//
		indices[i] = -1;

		// Search condition names for a match.
		//
		for (j = 0; j < (int)conditionNames.length(); ++j)
		{
			if (conditions[i] == conditionNames[j])
			{
				// Found a match.  Store the index and stop looking for
				//
				indices[i] = j;
				break;
			}
		}
	}

	for (i = 0; i < (int)conditions.length(); ++i)
	{
		j = indices[i];
		if (j == -1)
		{
			MGlobal::displayWarning(conditions[i] +
									MString("is not a valid condition name\n"));
			break;
		}

		if (addMessage && callbackId[j] == 0)
		{
			callbackId[j] = MConditionMessage::addConditionCallback(
				conditions[i],
				conditionChangedCB,
				(void*)(size_t)j,
				&status);
			
			if (!status)
			{
				status.perror("failed to add callback for " + conditions[i]);
				callbackId[j] = 0;
			}
		}
		else if (delMessage && callbackId[j] != 0)
		{
			status = MMessage::removeCallback(callbackId[j]);

			if (!status)
			{
				status.perror("failed to remove callback for " + conditions[i]);
			}

			callbackId[j] = 0;
		}
	}

	// Ok, we've made all the necessary changes.  Now show the status.
	//

	MGlobal::displayInfo("Condition Name        State  Msgs On\n");
	MGlobal::displayInfo("--------------------  -----  -------\n");

	char tmpStr[128];
	bool state, msgs;
	
	for (i = 0; i < (int)conditions.length(); ++i)
	{
		j = indices[i];
		if (j == -1)
		{
			continue;
		}

		state = MConditionMessage::getConditionState(conditions[i], &status);
		if (!status)
		{
			status.perror("failed to get status for " + conditions[i]);
			state = false;
		}

		msgs = (callbackId[j] != 0);

		sprintf(tmpStr, "%-20s  %-5s  %s\n",
				conditions[i].asChar(),
				state ? "true" : "false",
				msgs  ? "yes"  : "no");

		MGlobal::displayInfo(tmpStr);
	}

	// Free up the indices we allocated.
	//
	delete [] indices;

	return status;
}

///////////////////////////////////////////////////
//
// Plug-in functions
//
///////////////////////////////////////////////////

MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = MConditionMessage::getConditionNames(conditionNames);
	if (!status)
	{
		return status;
	}

	cout<<"conditionTest: "<<conditionNames.length()<<" conditions are defined."<<endl;

	callbackId = new MCallbackId [conditionNames.length()];

	if (!callbackId)
	{
		return MStatus(MS::kInsufficientMemory);
	}

	for (unsigned int i = 0; i < conditionNames.length(); ++i)
	{
		callbackId[i] = 0;
	}

	// Register the command so we can actually do some work
	//
	status = plugin.registerCommand("conditionTest",
									conditionTest::creator,
									conditionTest::newSyntax);

	if (!status)
	{
		status.perror("registerCommand");
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus   status;
	MFnPlugin plugin( obj );

	// Loop through all the ids and remove the callbacks
	//
	int i;
	int len = conditionNames.length();

	for (i = 0; i < len; ++i)
	{
		if (callbackId[i] != 0)
		{
			MGlobal::displayWarning("Removing callback for " +
									conditionNames[i] +
									"\n");
			MMessage::removeCallback(callbackId[i]);
			callbackId[i] = 0;
		}
	}

	conditionNames.clear();

	delete [] callbackId;

	// Deregister the command
	//
	status = plugin.deregisterCommand("conditionTest");

	if (!status)
	{
		status.perror("deregisterCommand");
	}

	return status;
}

///////////////////////////////////////////////////
//
// Callback function
//
///////////////////////////////////////////////////

static void conditionChangedCB(bool state, void * data)
{
	int i = (int)(size_t)data;

	if (i > 0 && i < (int)conditionNames.length())
	{
		MGlobal::displayInfo("condition " +
							 conditionNames[i] +
							 " changed to " +
							 (state ? "true\n" : "false\n"));
	}
	else
	{
		MGlobal::displayWarning("BOGUS client data in conditionChangedCB!\n");
	}
}
