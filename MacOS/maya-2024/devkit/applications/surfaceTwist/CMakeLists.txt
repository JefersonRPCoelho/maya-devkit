#-
# ==========================================================================
# Copyright (c) 2018 Autodesk, Inc.
# All rights reserved.
# 
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by Autodesk, Inc., and are
# protected by Federal copyright law. They may not be disclosed to third
# parties or copied or duplicated in any form, in whole or in part, without
# the prior written consent of Autodesk, Inc.
# ==========================================================================
#+


cmake_minimum_required(VERSION 3.13)

# specify project name
set(PROJECT_NAME surfaceTwist)

project(${PROJECT_NAME})

# include the project setting file
include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)


set(SOURCE_FILES
   surfaceTwist.cpp
)

set(LIBRARIES
	OpenMaya
	Foundation
)

# build application
build_application()
