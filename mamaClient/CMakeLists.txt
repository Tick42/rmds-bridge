cmake_minimum_required(VERSION 2.6)

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

project (mamalistenc)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

set (INCLUDE_LIBRARIES_LIST
	${CMAKE_CURRENT_SOURCE_DIR}
	${OPENMAMA_INCLUDE_DIR}
	$(OPENMAMACPP_INCLUDE_DIR)
	${OPENMAMA_COMMON_INCLUDE_DIR}
	${WOMBAT_OS_DEPENDENT_INCLUDE_DIR}
	)

if (NOT MSVC)
	LIST(APPEND INCLUDE_LIBRARIES_LIST ${UUID_INCLUDE_DIR} ) 
endif (NOT MSVC)

include_directories(
	${INCLUDE_LIBRARIES_LIST}
	)

add_executable (mamalistenc
		mamalistenc.c		
		config.h.in
	)

set (
	LINK_LIBRARIES_LIST 
	${OPENMAMA_LIBRARIES}
	${OPENMAMACPP_LIBRARIES}
	${OPENMAMA_COMMON_LIBRARIES}
	)	
	

target_link_libraries(	
	mamalistenc 
	${LINK_LIBRARIES_LIST}
	)


# mamalisten cpp
add_executable (mamalistencpp
		mamalistencpp.cpp		
		config.h.in
	)


target_link_libraries(	
	mamalistencpp 
	${LINK_LIBRARIES_LIST}
	)

install(TARGETS mamalistenc
  # IMPORTANT: Add the mamalistenc executable to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)

