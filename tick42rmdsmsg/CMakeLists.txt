#cmake_minimum_required(VERSION 2.8)

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

project (mamatick42rmdsmsgimpl)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${OPENMAMA_INCLUDE_DIR} 
	${OPENMAMA_COMMON_INCLUDE_DIR} 
	${Boost_INCLUDE_DIRS}
	${UPAAPI_CPP_INCLUDE_DIR} 
	${WOMBAT_OS_DEPENDENT_INCLUDE_DIR}
	${PROJECT_ROOT_DIR}/tick42rmds
	${PROJECT_ROOT_DIR}/utils
	)

add_library(mamatick42rmdsmsgimpl SHARED	
	MamaDateTimeWrapper.cpp
	MamaMsgWrapper.cpp
	MamaPriceWrapper.cpp
	upamsgimpl.cpp
	upamsgutils.cpp
	upapayloadimpl.cpp
			
	MamaDateTimeWrapper.h
	MamaMsgWrapper.h
	MamaPriceWrapper.h
	upafieldpayload.h
	upamsgimpl.h
	upamsgutils.h
	upapayload.h
	upapayloadimpl.h
	upavaluetype.h
		
	config.h.in	
	)

set (
	LINK_LIBRARIES_LIST 
	${OPENMAMA_LIBRARIES} 
	${Boost_FILESYSTEM_LIBRARY} 
	${Boost_DATE_TIME_LIBRARY}
	${OPENMAMA_COMMON_LIBRARIES}
	${UPA_CPP_LIBRARIES} 
	${UTILS_BINARY_DIR}
	)	

target_link_libraries(	
	mamatick42rmdsmsgimpl 
	${LINK_LIBRARIES_LIST}
	)

install(TARGETS mamatick42rmdsmsgimpl
  # IMPORTANT: Add the mamatick42rmdsimpl library to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/tick42rmdsmsg"
    COMPONENT dev)

