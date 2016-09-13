cmake_minimum_required(VERSION 2.8)

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

project(utils)

set (INCLUDE_LIBRARIES_LIST 
	${CMAKE_CURRENT_SOURCE_DIR}
	${OPENMAMA_INCLUDE_DIR}
	${OPENMAMA_COMMON_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
	${WOMBAT_OS_DEPENDENT_INCLUDE_DIR}
	)

if (MSVC)
	LIST(APPEND INCLUDE_LIBRARIES_LIST ${UUID_INCLUDE_DIR} ) 
endif (MSVC)
	
include_directories(${INCLUDE_LIBRARIES_LIST})

set (${PROJECT_NAME}_HEADERS 
	utils/environment.h
	utils/filesystem.h
	utils/mama/mamaDictionaryWrapper.h
	utils/mama/mamaFieldDescriptorWrapper.h
	utils/mama/types.h
	utils/mama.h
	utils/os.h
	utils/properties.h
	utils/stdio_port.h
	utils/stdlib_port.h
	utils/t42log.h
	utils/thread.h
	utils/thread/interlockedInt.h
	utils/thread/lock.h
	utils/thread/semaphore.h
	utils/time.h
	utils/parse.h

	)

set (${PROJECT_NAME}_SOURCES 
	utils/environment.cpp
	utils/filesystem.cpp
	utils/mama/mamaDictionaryWrapper.cpp
	utils/mama/mamaFieldDescriptorWrapper.cpp
	utils/os.cpp
	utils/properties.cpp
	utils/t42log.cpp
	utils/parse.cpp
	
	${${PROJECT_NAME}_HEADERS}
	
	config.h.in	
	)

add_library(utils SHARED
	${${PROJECT_NAME}_SOURCES}
	)
	
 set (
 	LINK_LIBRARIES_LIST 
 	${Boost_FILESYSTEM_LIBRARY} 
 	${Boost_DATE_TIME_LIBRARY}
 	${OPENMAMA_LIBRARIES} 
 	${OPENMAMA_COMMON_LIBRARIES}
 	)

target_link_libraries(
	${PROJECT_NAME} 
	${LINK_LIBRARIES_LIST}
	)
	
set_target_properties(utils PROPERTIES
  PUBLIC_HEADER "${${PROJECT_NAME}_SOURCES};${CMAKE_CURRENT_BINARY_DIR}/config.h")

#mark_as_advanced (CLEAR UTILS_INCLUDE_DIR)

install(TARGETS utils
  # IMPORTANT: Add the utils library to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
#  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT arlib
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/utils"
    COMPONENT dev)
