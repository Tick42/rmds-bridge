cmake_minimum_required (VERSION 2.8)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build CACHE STRING "Default out-source library" )
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds are not allowed. Please make a new directory (e.g. called build) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

if (NOT RMDS_ENHANCED)
  message(STATUS "Build Regular bridge. For Enhanced bridge set RMDS_ENHANCED on (True).")
  set(RMDS_ENHANCED FALSE CACHE BOOL "Build Regular bridge by default")
endif (NOT RMDS_ENHANCED)


set (CMAKE_VERBOSE_MAKEFILE ON)

# Set a default build type if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

project(RmdsBridge)

set(RMDSBRIDGE_MAJOR_VERSION 0)
set(RMDSBRIDGE_MINOR_VERSION 1)
set(RMDSBRIDGE_PATCH_VERSION 0)

set(RMDSBRIDGE_VERSION ${RMDSBRIDGE_MAJOR_VERSION}.${RMDSBRIDGE_MINOR_VERSION}.${RMDSBRIDGE_PATCH_VERSION})

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installationlink directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")

if (WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/CMake/RmdsBridge)
endif ()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if (NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif ()
endforeach()

message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
set( PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#--------------------------------------------------------------------------------------------------
# Packages Prerequisites
#--------------------------------------------------------------------------------------------------

set (BOOST_ROOT $ENV{TICK42_BOOST})

#--------------------------------------------------------------------------------------------------
# Packages
#--------------------------------------------------------------------------------------------------

#--------------------------------------------------------------------------------------------------
# cmake source path (used for the includes and find packages.
#--------------------------------------------------------------------------------------------------

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include (CheckCXXCompilerFlag)
include (FindBoost)
include (FindOpenMAMA)
include (FindUpaAPI)
include (FindUuid)

find_package (OpenMAMA)
find_package (UpaAPI)
find_package (Uuid)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

#--------------------------------------------------------------------------------------------------
# Boost Libraries
#--------------------------------------------------------------------------------------------------

set(Boost_DIR "$ENV{TICK42_BOOST}")
message ("Boost root is " ${Boost_DIR})

set(Boost_MULTITHREADED TRUE)
set(Boost_USE_STATIC_LIBS TRUE CACHE BOOL "Boost libraries are linked statically (default)")

find_package(Boost 1.54 REQUIRED date_time filesystem system program_options random)

add_definitions(-DBOOST_FILESYSTEM_VERSION=3 -DBOOST_FILESYSTEM_NO_DEPRECATED)

#--------------------------------------------------------------------------------------------------
# Compiler C++11/0x support
#--------------------------------------------------------------------------------------------------

option(BOOST_NAMESPACES "Use of BOOST namespaces for c++0x support" FALSE)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.3)
   set (ENABLE_CXX11 "-std=c++0x")

   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7)
       set (ENABLE_CXX11 "-std=c++11")
   endif ()

   set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ENABLE_CXX11}")
else ()
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        set (BOOST_NAMESPACES TRUE CACHE)
    endif()
endif ()

if(${BOOST_NAMESPACES})
    add_definitions(-DBOOST_NAMESPACES)
else()
    add_definitions(-DSTD_NAMESPACES)
endif()

set (CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fPIC")

#--------------------------------------------------------------------------------------------------
# Compiler
#--------------------------------------------------------------------------------------------------

function(add_compiler_flag_if_supported flag name)
  check_cxx_compiler_flag (${flag} ${name})
  if (${name})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
  endif ()
endfunction()

set (PEDANTIC OFF CACHE BOOL "$Enable lots of warnings")

if (CMAKE_COMPILER_IS_GNUCXX)
#	add_compiler_flag_if_supported(-Werror werror)
    add_compiler_flag_if_supported(-Wwrite-strings wwrite_strings)
    add_compiler_flag_if_supported(-Wreturn-type wreturn_type)
    add_compiler_flag_if_supported(-Wunused-value wunused_value)
    add_compiler_flag_if_supported(-Wtrigraphs wtrigraphs)
    add_compiler_flag_if_supported(-Wmissing-braces wmissing_braces)
    add_compiler_flag_if_supported(-Wcomment wcomment)
    add_compiler_flag_if_supported(-Wtype-limits type_limits)
    add_compiler_flag_if_supported(-Wempty-body empty_body)
    add_compiler_flag_if_supported(-Waddress waddress)
    add_compiler_flag_if_supported(-fPIC fPIC)

    if (PEDANTIC)
        add_compiler_flag_if_supported(-Wall wall)
        add_compiler_flag_if_supported(-Wextra wextra)
        add_compiler_flag_if_supported(-Wuninitialized wuninitialized)
        add_compiler_flag_if_supported(-Wsign-compare wsign_compare)
        add_compiler_flag_if_supported(-Wchar-subscripts char_subscipts)
    endif (PEDANTIC)
endif (CMAKE_COMPILER_IS_GNUCXX)

#--------------------------------------------------------------------------------------------------
# Dump all variables - use it when you need some generated variables from packages
#--------------------------------------------------------------------------------------------------

set (DUMP_VARIABLES OFF CACHE BOOL "Dump all variables")
if (DUMP_VARIABLES)
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message("${_variableName}=${${_variableName}}")
    endforeach()
endif ()
#--------------------------------------------------------------------------------------------------
# Sub projects
#--------------------------------------------------------------------------------------------------

#add_subdirectory(utils)
add_subdirectory(tick42rmds)
if (EXISTS "UPA Samples/Examples/rsslProvider")
    add_subdirectory("UPA Samples/Examples/rsslProvider")
endif ()
add_subdirectory(tick42rmdsmsg)
add_subdirectory(mamaClient)

# Add all targets to the build-tree export set
export(TARGETS mamatick42rmdsimpl mamatick42rmdsmsgimpl mamalistenc #utils
       FILE "${PROJECT_BINARY_DIR}/RmdsBridgeTargets.cmake")

#--------------------------------------------------------------------------------------------------
# Project settings
#--------------------------------------------------------------------------------------------------

# The interesting stuff goes here
# ===============================

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE RmdsBridge)

# Create the RmdsBridgeConfig.cmake and RmdsBridgeConfigVersion files
file(RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_CMAKE_DIR}"
   "${INSTALL_INCLUDE_DIR}")
# ... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(RmdsBridgeConfig.cmake.in
  "${PROJECT_BINARY_DIR}/RmdsBridgeConfig.cmake" @ONLY)
# ... for the install tree
set(CONF_INCLUDE_DIRS "/${RmdsBridge_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file(RmdsBridgeConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/RmdsBridgeConfig.cmake" @ONLY)
# ... for both
configure_file(RmdsBridgeConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/RmdsBridgeConfigVersion.cmake" @ONLY)

# Install the RmdsBridgeConfig.cmake and RmdsBridgeConfigVersion.cmake
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/RmdsBridgeConfig.cmake"
  "${PROJECT_BINARY_DIR}/RmdsBridgeConfigVersion.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)

message("INSTALL_CMAKE_DIR=${INSTALL_CMAKE_DIR}")

# Install the export set for use with the install-tree
install(
    EXPORT RmdsBridgeTargets DESTINATION
    "${INSTALL_CMAKE_DIR}" COMPONENT dev)
