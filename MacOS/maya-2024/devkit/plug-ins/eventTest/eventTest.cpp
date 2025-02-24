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

////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
// 
// Produces the MEL command "eventTest".
//
// The eventTest plug-in is a simple plug-in that displays the "events" occurring in Maya.
// An event in Maya that occurs at a specific point in time is of interest to the Maya internals or plug-ins.
// For example, "SelectionChanged" and "timeChanged" are two events.
// These events can be tracked at the MEL level with the -event flag to the "scriptJob" command. 
//
// The plug-in adds an "eventTest" command that lets you know which events are available, track specific events,
// and know which events are being tracked. The basic command syntax is: 
//
//	eventTest [-m on|off] [eventName ...]
//
// The "eventName" arguments must be the names of events. If no names are specified, the command
// will operate on all available events.
//
// If you use the -m flag, you can specify whether the plug-in must track messages for the specified events.
// If you specify the -m flag without any event names, it will turn on or turn off tracking for all events.
// For example:
//
//	mel: eventTest -m 1 timeChanged
//	Event Name Msgs On
//	-------------------- -------
//	timeChanged yes
// 
// After this example, the plug-in will display the following each time when the current time changes:
//
//	event timeChanged occurred
//
// You can disable all event tracking with the command:
//
//	eventTest -m off; 
//
////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MEventMessage.h>
#include <maya/MFnPlugin.h>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

///////////////////////////////////////////////////
//
// Definitions
//
///////////////////////////////////////////////////

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
static void eventCB(void * data);

// Array of callback ids.
//
typedef MCallbackId* MCallbackIdPtr;
static MCallbackIdPtr callbackId = NULL;

// Array of event names.
//
static MStringArray eventNames;

///////////////////////////////////////////////////
//
// Command class declaration
//
///////////////////////////////////////////////////
class eventTest : public MPxCommand
{
public:
	eventTest();
	                ~eventTest() override; 

	MStatus                 doIt( const MArgList& args ) override;

	static MSyntax			newSyntax();
	static void*			creator();

private:
	MStatus                 parseArgs( const MArgList& args );

	bool					addMessage;
	bool					delMessage;

	MStringArray			events;
};


///////////////////////////////////////////////////
//
// Command class implementation
//
///////////////////////////////////////////////////

// Constructor
//
eventTest::eventTest()
:	addMessage(false)
,	delMessage(false)
{
	events.clear();
}

// Destructor
//
eventTest::~eventTest()
{
	// Do nothing
}

// creator
//
void* eventTest::creator()
{
	return (void *) (new eventTest);
}

// newSyntax
//
MSyntax eventTest::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag(kMessageFlag, kMessageFlagLong, MSyntax::kBoolean);
	syntax.setObjectType(MSyntax::kStringObjects);

	return syntax;
}

// parseArgs
//
MStatus eventTest::parseArgs(const MArgList& args)
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

	status = argData.getObjects(events);
	if (!status)
	{
		status.perror("could not parse event names");
	}

	// If there are no events specified, operate on all of them
	//
	if (events.length() == 0)
	{
		// eventNames is set in initializePlugin to all the
		// currently available event names.
		//
		events = eventNames;
	}

	return status;
}

// doIt
//
MStatus eventTest::doIt(const MArgList& args)
{
	MStatus status;

	status = parseArgs(args);
	if (!status)
	{
		return status;
	}

	// Allocate an array of indices.  events[n] is a user provided
	// event name.  Look it up in the static eventNames array
	// and set indices[n] to the index of the entry in eventNames.
	//
	// This maps the user specified events to the global events
	// so we can track callback adds and removes globally.
	//
	int * indices = new int [events.length()];

	int i, j;

	for (i = 0; i < (int)events.length(); ++i)
	{
		// Initialize the entry to "not found".
		//
		indices[i] = -1;

		// Search event names for a match.
		//
		for (j = 0; j < (int)eventNames.length(); ++j)
		{
			if (events[i] == eventNames[j])
			{
				// Found a match.  Store the index and stop looking for
				//
				indices[i] = j;
				break;
			}
		}
	}

	for (i = 0; i < (int)events.length(); ++i)
	{
		j = indices[i];
		if (j == -1)
		{
			MGlobal::displayWarning(events[i] +
									MString("is not a valid event name\n"));
			break;
		}

		if (addMessage && callbackId[j] == 0)
		{
			callbackId[j] = MEventMessage::addEventCallback(
				events[i],
				eventCB,
				(void *)(size_t)j,
				&status);
			
			if (!status)
			{
				status.perror("failed to add callback for " + events[i]);
				callbackId[j] = 0;
			}
		}
		else if (delMessage && callbackId[j] != 0)
		{
			status = MMessage::removeCallback(callbackId[j]);

			if (!status)
			{
				status.perror("failed to remove callback for " + events[i]);
			}

			callbackId[j] = 0;
		}
	}

	// Ok, we've made all the necessary changes.  Now show the status.
	//

	MGlobal::displayInfo("Event Name            Msgs On\n");
	MGlobal::displayInfo("--------------------  -------\n");

	char tmpStr[128];
	bool msgs;
	
	for (i = 0; i < (int)events.length(); ++i)
	{
		j = indices[i];
		if (j == -1)
		{
			continue;
		}

		msgs = (callbackId[j] != 0);

		sprintf(tmpStr, "%-20s  %s\n",
				events[i].asChar(),
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

	status = MEventMessage::getEventNames(eventNames);
	if (!status)
	{
		return status;
	}

	// Search for and remove idle and idleHigh events since they will
	// completely swamp the output.  They are tested by the idleTest
	// plug-in
	//

	int i;

	for (i = 0; i < (int)eventNames.length(); ++i)
	{
		if (eventNames[i] == "idle" ||
			eventNames[i] == "idleHigh")
		{
			eventNames.remove(i);
			--i;
		}
	}
	
	cout<<"eventTest: "<<eventNames.length()<<" events are defined"<<endl;

	callbackId = new MCallbackId [eventNames.length()];

	if (!callbackId)
	{
		return MStatus(MS::kInsufficientMemory);
	}

	for (i = 0; i < (int)eventNames.length(); ++i)
	{
		callbackId[i] = 0;
	}

	// Register the command so we can actually do some work
	//
	status = plugin.registerCommand("eventTest",
									eventTest::creator,
									eventTest::newSyntax);

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
	int len = eventNames.length();

	for (i = 0; i < len; ++i)
	{
		if (callbackId[i] != 0)
		{
			MGlobal::displayWarning("Removing callback for " +
									eventNames[i] +
									"\n");
			MMessage::removeCallback(callbackId[i]);
			callbackId[i] = 0;
		}
	}

	eventNames.clear();

	delete [] callbackId;

	// Deregister the command
	//
	status = plugin.deregisterCommand("eventTest");

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

static void eventCB(void * data)
{
	int i = (int)(size_t)data;

	if (i >= 0 && i < (int)eventNames.length())
	{
		MGlobal::displayInfo("event " +
							 eventNames[i] +
							 " occurred\n");
	}
	else
	{
		MGlobal::displayWarning("BOGUS client data in eventCB!\n");
	}
}
