# - Find Direct3D 8
# Find the Direct3D8 includes and libraries
#
# D3D8_INCLUDE_DIR - where to find d3d8.h
# D3D8_LIBRARIES - List of libraries when using D3D8.
# D3D8_FOUND - True if D3D8 found.

if(D3D8_INCLUDE_DIR)
  # Already in cache, be silent
  set(D3D8_FIND_QUIETLY TRUE)
endif(D3D8_INCLUDE_DIR)

find_path     (D3D8_INCLUDE_DIR       d3d8.h          HINTS "$ENV{DXSDK_DIR}/Include" )
find_library  (D3D8_LIBRARY     NAMES libd3d8  d3d8   HINTS "$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}" )
find_library  (D3DX8_LIBRARY    NAMES libd3dx8 d3dx8  HINTS "$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}" )

# Handle the QUIETLY and REQUIRED arguments and set D3D8_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(D3D8 DEFAULT_MSG D3D8_LIBRARY D3D8_INCLUDE_DIR)

if(D3D8_FOUND)
  set(D3D8_LIBRARIES ${D3D8_LIBRARY} ${D3DX8_LIBRARY})
else(D3D8_FOUND)
  set(D3D8_LIBRARIES)
endif(D3D8_FOUND)

mark_as_advanced(D3D8_INCLUDE_DIR D3D8_LIBRARIES) 
