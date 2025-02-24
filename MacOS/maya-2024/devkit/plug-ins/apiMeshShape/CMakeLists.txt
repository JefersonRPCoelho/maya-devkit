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

# include the project setting file
include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

# specify project name
set(PROJECT_NAME apiMeshShape)

#set .mel file list
set(MEL_FILES 
    apiMeshShape.mel)


# set SOURCE_FILES
set(SOURCE_FILES
   apiMeshShape.cpp
   apiMeshIterator.cpp
   apiMeshGeom.cpp
   apiMeshData.cpp
   apiMeshCreator.cpp
   apiMeshGeometryOverride.cpp
   apiMeshSubSceneOverride.cpp
   api_macros.h
   apiMeshShape.h
   apiMeshIterator.h
   apiMeshGeom.h
   apiMeshData.h
   apiMeshCreator.h
   apiMeshGeometryOverride.h
   apiMeshSubSceneOverride.h
)

# set linking libraries
set(LIBRARIES
     OpenMaya
     OpenMayaUI
     OpenMayaRender
     Foundation
)

find_opengl()
find_glew()


if(GLEW_FOUND)

# Build plugin
build_plugin()

endif()


