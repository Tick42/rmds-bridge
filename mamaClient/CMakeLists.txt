cmake_minimum_required(VERSION 2.8)

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

project (mamalistenc)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (INCLUDE_LIBRARIES_LIST
	${CMAKE_CURRENT_SOURCE_DIR}
	${OPENMAMA_INCLUDE_DIR}
    ${OPENMAMACPP_INCLUDE_DIR}
	${OPENMAMA_COMMON_INCLUDE_DIR}
	${WOMBAT_OS_DEPENDENT_INCLUDE_DIR}
	)

if (NOT MSVC)
    list (APPEND INCLUDE_LIBRARIES_LIST ${UUID_INCLUDE_DIR})
endif (NOT MSVC)

include_directories(
	${INCLUDE_LIBRARIES_LIST}
	)

# mamalistenc
add_executable (mamalistenc
		mamalistenc.c		
		config.h.in
	)

set (LINK_LIBRARIES_LIST
	${OPENMAMA_LIBRARIES}
	${OPENMAMA_COMMON_LIBRARIES}
	)	
	

target_link_libraries (mamalistenc
	${LINK_LIBRARIES_LIST}
	)


install(TARGETS mamalistenc
  # IMPORTANT: Add the mamalistenc executable to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)


# mamalisten cpp
add_executable (mamalistencpp
        mamalistencpp.cpp
        config.h.in
    )

target_link_libraries(
    mamalistencpp
    ${LINK_LIBRARIES_LIST}
    ${OPENMAMACPP_LIBRARIES}
    )

# mamapublisherc
add_executable (mamapublisherc
        mamapublisherc.c
        config.h.in
    )


target_link_libraries (mamapublisherc
    ${LINK_LIBRARIES_LIST}
    )


install(TARGETS mamapublisherc
  # IMPORTANT: Add the mamapublisherc executable to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)

# mamaproxyc
add_executable (mamaproxyc
        mamaproxyc.c
        config.h.in
    )


target_link_libraries (mamaproxyc
    ${LINK_LIBRARIES_LIST}
    )


install(TARGETS mamaproxyc
  # IMPORTANT: Add the mamaproxyc executable to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)

# mamachurnc
add_executable (mamachurnc
        mamachurnc.c
        config.h.in
    )


target_link_libraries (mamachurnc
    ${LINK_LIBRARIES_LIST}
    )


install(TARGETS mamachurnc
  # IMPORTANT: Add the mamaproxyc executable to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin)
