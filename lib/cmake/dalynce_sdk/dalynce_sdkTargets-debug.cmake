#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "dalynce::sdk" for configuration "Debug"
set_property(TARGET dalynce::sdk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(dalynce::sdk PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/dalynce_sdk.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/dalynce_sdk.dll"
  )

list(APPEND _cmake_import_check_targets dalynce::sdk )
list(APPEND _cmake_import_check_files_for_dalynce::sdk "${_IMPORT_PREFIX}/lib/dalynce_sdk.lib" "${_IMPORT_PREFIX}/bin/dalynce_sdk.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
