#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "PySide2::pyside2" for configuration "Release"
set_property(TARGET PySide2::pyside2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(PySide2::pyside2 PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "Shiboken2::libshiboken;Qt5::Qml;Qt5::Core"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libpyside2.cpython-310-darwin.5.15.2.1.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libpyside2.cpython-310-darwin.5.15.dylib"
  )

list(APPEND _cmake_import_check_targets PySide2::pyside2 )
list(APPEND _cmake_import_check_files_for_PySide2::pyside2 "${_IMPORT_PREFIX}/lib/libpyside2.cpython-310-darwin.5.15.2.1.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
