# Remark: make sure that you ran the LinuxSoLink script (resides on the root of the UPA libs) to make the correct shared object names!

find_path (UPAAPI_CPP_INCLUDE_DIR
        NAMES rtr/rsslDataTypeEnums.h
        PATH_SUFFIXES
            Include
        PATHS
            $ENV{TICK42_UPA}
        )

find_path (UPAAPI_CPP_ANSI_INCLUDE_DIR
        NAMES ansi/q_ansi.h
        PATH_SUFFIXES
            Utils/Ansi/Include
        PATHS
            $ENV{TICK42_UPA}
        )

IF (NOT UPA_CPP_LIBRARIES_LINK_STATIC)
    set (UPA_CPP_LIBRARIES_LINK_STATIC OFF CACHE BOOL "Enable static linking of UPA libraries. Defaulted to link against shared (.so) libraries" FORCE)
ENDIF (NOT UPA_CPP_LIBRARIES_LINK_STATIC)

if (WIN32 AND NOT CYGWIN)
    # Create a path made of concatenated strings that reflect the OS (32/64) target architecture and the build target (Debug/Release)

    # Compiler Target Architecture
    # The path to the right complier + the right architecture (32/64) - only GCC VS100 and higher is supported.
    if ( CMAKE_SIZEOF_VOID_P EQUAL 8 ) #64 bit
        set( WIN_ARCH_VC100 "WIN_64_VS100" )
    else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
        set( WIN_ARCH_VC100 "WIN_32_VS100" )
    endif ( CMAKE_SIZEOF_VOID_P EQUAL 8 )

    # Compiler target build (Debug/Release etc.)
    if ( CMAKE_BUILD_TYPE STREQUAL "Debug")
        set ( UPA_LIB_TYPE "Debug_MDd")
        #add_definitions( -DCOMPILE_64BITS) fixme: test it over
    else (CMAKE_BUILD_TYPE STREQUAL "Debug")
        if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            set ( UPA_LIB_TYPE "Release_MD_Assert")
        else (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            set ( UPA_LIB_TYPE "Release_MD")
        endif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    endif ( CMAKE_BUILD_TYPE STREQUAL "Debug")

    # Full path to the right shared object considering the target architecture and target build
    set( UPA_LIB_DIR "$ENV{TICK42_UPA}\\Libs\\${WIN_ARCH_VC100}\\${UPA_LIB_TYPE}")

#find_path (UPA_CPP_LIBRARIES_PATH NAME librsslData.lib PATHS ${UPA_LIB_DIR})

    if (UPA_CPP_LIBRARIES_LINK_STATIC)
        #------------------------------------------------------------------------------------------------------------------------------
        # Build Type: Static Libraries
        #------------------------------------------------------------------------------------------------------------------------------

        find_library (UPA_CPP_LIBRARIES_DATA NAME librsslData.lib PATHS ${UPA_LIB_DIR})
        find_library (UPA_CPP_LIBRARIES_MESSAGES NAME librsslMessages PATHS ${UPA_LIB_DIR})
        find_library (UPA_CPP_LIBRARIES_TRANSPORT NAME librsslTransport.lib PATHS ${UPA_LIB_DIR})

        set (UPA_CPP_LIBRARIES ${UPA_CPP_LIBRARIES_DATA} ${UPA_CPP_LIBRARIES_MESSAGES} ${UPA_CPP_LIBRARIES_TRANSPORT})
    else (UPA_CPP_LIBRARIES_LINK_STATIC)
        #------------------------------------------------------------------------------------------------------------------------------
        # Build Type: Shared Libraries
        #------------------------------------------------------------------------------------------------------------------------------

        # The shared version of rssl libraries is used
        set (UPA_LIB_SO "librssl.lib")
        find_library (UPA_CPP_LIBRARIES_SO NAMES ${UPA_LIB_SO} PATHS ${UPA_LIB_DIR}/Shared )

        set (UPA_CPP_LIBRARIES ${UPA_CPP_LIBRARIES_SO} )
    endif (UPA_CPP_LIBRARIES_LINK_STATIC)

    if (MSVC)
        list (APPEND UPA_CPP_LIBRARIES winInet ws2_32)
    endif (MSVC)

ELSE (WIN32 AND NOT CYGWIN) # This one is ONLY linux see comment below (UPA basically support also Sun)

#------------------------------------------------------------------------------------------------------------------------------
# General Definitions
#------------------------------------------------------------------------------------------------------------------------------

    # Add the definition Linux to support the libraries
    add_definitions(-DLinux -Dx86_Linux_4X -Dx86_Linux_5X -DLinuxVersion=5 -D_iso_stdcpp_ -D_POSIX_SOURCE=1 -D_POSIX_C_SOURCE=199506L -D_XOPEN_SOURCE=500  -D_POSIX_PTHREAD_SEMANTICS -D_GNU_SOURCE -D_DEFAULT_SOURCE)

    # set -D_SVID_SOURCE=1 #todo: check it out later
    add_definitions( -D_SVID_SOURCE=1)

#------------------------------------------------------------------------------------------------------------------------------
# Architecture Related: Definitions and Libraries Path Helpers
#------------------------------------------------------------------------------------------------------------------------------

# Compiler Target Architecture
# The path to the right complier + the right architecture (32/64)
    if ( CMAKE_SIZEOF_VOID_P EQUAL 8 ) #64 bit
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.4)
            set( OS_ARCH_GCC_COMBINATION "RHEL5_64_GCC412" )
            add_definitions( -DCOMPILE_64BITS)
        else (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.4)
            set( OS_ARCH_GCC_COMBINATION "RHEL6_64_GCC444" )
            add_definitions( -DCOMPILE_64BITS)
        endif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.4)
    else( CMAKE_SIZEOF_VOID_P EQUAL 8 ) #32 bit
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.4)
            set( OS_ARCH_GCC_COMBINATION "RHEL5_32_GCC412" )
        else (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.4)
            set( OS_ARCH_GCC_COMBINATION "RHEL6_32_GCC444" )
        endif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4.4)
    endif ( CMAKE_SIZEOF_VOID_P EQUAL 8 )

#------------------------------------------------------------------------------------------------------------------------------
# Build Type: Libraries Path Helpers
#------------------------------------------------------------------------------------------------------------------------------

    if ( CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set ( UPA_LIB_TYPE "Optimized_Assert")
    else ( CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set ( UPA_LIB_TYPE "Optimized")
    endif ( CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")

#------------------------------------------------------------------------------------------------------------------------------
# Build Type: Libraries Path
#------------------------------------------------------------------------------------------------------------------------------
    # Full path to the right shared object considering the target architecture and target build
    set( UPA_LIB_DIR "$ENV{TICK42_UPA}/Libs/${OS_ARCH_GCC_COMBINATION}/${UPA_LIB_TYPE}")

    set( UPA_ANSI_DIR "$ENV{TICK42_UPA}/Utils/Ansi/Libs/${OS_ARCH_GCC_COMBINATION}/Optimized")
    set (UPA_LIB_ANSI "libansi.a")
    find_library (UPA_CPP_LIBRARIES_ANSI NAMES ${UPA_LIB_ANSI} PATHS ${UPA_ANSI_DIR})

    if (UPA_CPP_LIBRARIES_LINK_STATIC)
        #------------------------------------------------------------------------------------------------------------------------------
        # Build Type: Static Libraries`
        #------------------------------------------------------------------------------------------------------------------------------

        # The static version of rssl libraries is used
        find_library (UPA_CPP_LIBRARIES_DATA NAME librsslData.a PATHS ${UPA_LIB_DIR})
        find_library (UPA_CPP_LIBRARIES_MESSAGES NAME librsslMessages.a PATHS ${UPA_LIB_DIR})
        find_library (UPA_CPP_LIBRARIES_TRANSPORT NAME librsslTransport.a PATHS ${UPA_LIB_DIR})

        set (UPA_CPP_LIBRARIES ${UPA_CPP_LIBRARIES_DATA} ${UPA_CPP_LIBRARIES_MESSAGES} ${UPA_CPP_LIBRARIES_TRANSPORT})
        set (UPA_CPP_LIBRARIES_PATH ${UPA_CPP_LIBRARIES} ${UPA_CPP_LIBRARIES_ANSI})
    else (UPA_CPP_LIBRARIES_LINK_STATIC)

        #------------------------------------------------------------------------------------------------------------------------------
        # Build Type: Shared Libraries
        #------------------------------------------------------------------------------------------------------------------------------

        # The shared version of rssl libraries is used
        set (UPA_LIB_SO "librssl.so")
        find_library (UPA_CPP_LIBRARIES_SO NAMES ${UPA_LIB_SO} PATHS ${UPA_LIB_DIR}/Shared )

        set (UPA_CPP_LIBRARIES ${UPA_CPP_LIBRARIES_SO} ${UPA_CPP_LIBRARIES_ANSI})
        set (UPA_CPP_LIBRARIES_PATH ${UPA_CPP_LIBRARIES})
    endif (UPA_CPP_LIBRARIES_LINK_STATIC)


#------------------------------------------------------------------------------------------------------------------------------
# UPA Dependencies:
#------------------------------------------------------------------------------------------------------------------------------

# more dependencies and -l flags to be used (see rt dl m that are -lrt -ldl -lm in accordance only with gcc kind of compiler)
    set (UPA_CPP_DEPEND_SYSTEM_LIBS nsl pthread rt dl m)
    list( APPEND UPA_CPP_LIBRARIES ${UPA_CPP_DEPEND_SYSTEM_LIBS})

ENDif (WIN32 AND NOT CYGWIN)


#include (FindPackageHandleStandardArgs)

#mark_as_advanced (CLEAR UPA_CPP_LIBRARIES, UPAAPI_CPP_INCLUDE_DIR)
find_package_handle_standard_args (UpaAPI DEFAULT_MSG UPA_CPP_LIBRARIES UPAAPI_CPP_INCLUDE_DIR UPAAPI_CPP_ANSI_INCLUDE_DIR UPA_CPP_LIBRARIES_PATH)
