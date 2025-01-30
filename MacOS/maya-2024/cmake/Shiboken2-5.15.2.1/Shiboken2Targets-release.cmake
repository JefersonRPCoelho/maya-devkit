#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Shiboken2::libshiboken" for configuration "Release"
set_property(TARGET Shiboken2::libshiboken APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Shiboken2::libshiboken PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libshiboken2.cpython-310-darwin.5.15.2.1.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libshiboken2.cpython-310-darwin.5.15.dylib"
  )

list(APPEND _cmake_import_check_targets Shiboken2::libshiboken )
list(APPEND _cmake_import_check_files_for_Shiboken2::libshiboken "${_IMPORT_PREFIX}/lib/libshiboken2.cpython-310-darwin.5.15.2.1.dylib" )

# Import target "Shiboken2::shiboken2" for configuration "Release"
set_property(TARGET Shiboken2::shiboken2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Shiboken2::shiboken2 PROPERTIES
  IMPORTED_LOCATION_RELEASE "/Volumes/DATA/local/S/workspace/pyside_maya/src/pyside3_install/py3.10-qt5.15.2-64bit-release/bin/shiboken2"
  )

list(APPEND _cmake_import_check_targets Shiboken2::shiboken2 )
list(APPEND _cmake_import_check_files_for_Shiboken2::shiboken2 "/Volumes/DATA/local/S/workspace/pyside_maya/src/pyside3_install/py3.10-qt5.15.2-64bit-release/bin/shiboken2" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
