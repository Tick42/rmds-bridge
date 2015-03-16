IF (NOT WIN32 OR CYGWIN)
#used only in unices

	if (WIN32 AND NOT CYGWIN)
	# For Windows just fake the need for uuid library 
		#default #include library to current
		find_path (UUID_INCLUDE_DIR 
			NAMES
				uuid.h
			PATH_SUFFIXES 
				include
			PATHS
				$ENV{TICK42_UUID}
				/usr/local/include
				/usr/include
				/opt/local/include
				/opt/include
				/usr
			)

		#default the library object to nothing (won't link against nothing)
		set (UUID_LIBRARIES "") 
	ELSE (WIN32 AND NOT CYGWIN)
	# For other systems but native Windows
		find_path (UUID_INCLUDE_DIR 
			NAMES
				uuid.h
			PATH_SUFFIXES 
				include
				uuid
			PATHS
				$ENV{TICK42_UUID}
				/usr/local/include
				/usr/include
				/opt/local/include
				/opt/include
				/usr
			)

		find_library (UUID_LIBRARIES
			NAMES
				uuid
				ossp-uuid
			PATH_SUFFIXES 
				lib64 
				lib
			PATHS
				/usr/local
				/usr
				/opt/local
				/opt
			)
	ENDif (WIN32 AND NOT CYGWIN)

	include (FindPackageHandleStandardArgs)
	mark_as_advanced (CLEAR UUID_LIBRARIES, UUID_INCLUDE_DIR)
	find_package_handle_standard_args (Uuid DEFAULT_MSG UUID_LIBRARIES UUID_INCLUDE_DIR)

ENDIF (NOT WIN32 OR CYGWIN)
