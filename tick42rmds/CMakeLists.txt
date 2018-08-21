#cmake_minimum_required(VERSION 2.8)

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

project (tick42rmds)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

set (INCLUDE_LIBRARIES_LIST
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${OPENMAMA_INCLUDE_DIR}
   ${OPENMAMA_COMMON_INCLUDE_DIR}
   ${Boost_INCLUDE_DIRS}
   ${UPAAPI_CPP_INCLUDE_DIR}
   ${UPAAPI_CPP_ANSI_INCLUDE_DIR}
   ${WOMBAT_OS_DEPENDENT_INCLUDE_DIR}
   ${PROJECT_ROOT_DIR}/utils
   )
if (NOT MSVC)
   LIST(APPEND INCLUDE_LIBRARIES_LIST ${UUID_INCLUDE_DIR} )
endif (NOT MSVC)

include_directories(
   ${INCLUDE_LIBRARIES_LIST}
   )

set(SRC_FILES
   bridge.cpp
   inbox.cpp
   io.cpp
   msg.cpp
   publisher.cpp
   queue.cpp
   RMDSBridgeSubscription.cpp
   RMDSConnectionConfig.cpp
   RMDSFileSystem.cpp
   RMDSNIPublisher.cpp
   RMDSPublisher.cpp
   RMDSPublisherSource.cpp
   RMDSSource.cpp
   RMDSSources.cpp
   RMDSSubscriber.cpp
   StatisticsLogger.cpp
   subscription.cpp
   timer.cpp
   ToMamaFieldType.cpp
   transport.cpp
   UPAAsMamaFieldType.cpp
   UPABookMessage.cpp
   UPABridgePoster.cpp
   UPAConsumer.cpp
   UPADecodeUtils.cpp
   UPADictionary.cpp
   UPADictionaryWrapper.cpp
   UPALogin.cpp
   UPAMamaFieldMap.cpp
   UPAMessage.cpp
   UPANIProvider.cpp
   UPAPostManager.cpp
   UPAProvider.cpp
   UPAPublisherItem.cpp
   UPASourceDirectory.cpp
   UPAStreamManager.cpp
   UPASubscription.cpp
   UPATransportNotifier.cpp
   UPAFieldDecoder.cpp
   UPAFieldEncoder.cpp
   UPAMamaCommonFields.cpp

   # Headers (not necessary but good to have)
   ConnectionListener.h
   DictionaryReply.h
   inbox.h
   msg.h
   RMDSBridgeImpl.h
   RMDSBridgeSubscription.h
   rmdsBridgeTypes.h
   RMDSConnectionConfig.h
   rmdsdefs.h
   RMDSFileSystem.h
   RMDSNIPublisher.h
   RMDSPublisherBase.h
   RMDSPublisher.h
   RMDSPublisherSource.h
   RMDSSource.h
   RMDSSources.h
   RMDSSubscriber.h
   SourceDirectoryResponseListener.h
   SourceDirectoryTypes.h
   StatisticsLogger.h
   SubscriptionResponseListener.h
   tick42rmdsbridgefunctions.h
   ToMamaFieldType.h
   transportconfig.h
   transport.h
   UPAAsMamaFieldTypeEnum.h
   UPAAsMamaFieldType.h
   UPABookMessage.h
   UPABridgePoster.h
   UPAConsumer.h
   UPADecodeUtils.h
   UPADictionary.h
   UPADictionaryWrapper.h
   UPALogin.h
   UPAMamaFieldMap.h
   UPAMessage.h
   UPANIProvider.h
   UPAPostManager.h
   UPAProvider.h
   UPAPublisherItem.h
   UPASourceDirectory.h
   UPAStreamManager.h
   UPASubscription.h
   UPATransportNotifier.h
   UPAFieldDecoder.h
   UPAFieldEncoder.h
   UPAMamaCommonFields.h


   # compile utils directly
   ${PROJECT_ROOT_DIR}/utils/utils/environment.h
   ${PROJECT_ROOT_DIR}/utils/utils/filesystem.h
   ${PROJECT_ROOT_DIR}/utils/utils/mama.h
   ${PROJECT_ROOT_DIR}/utils/utils/os.h
   ${PROJECT_ROOT_DIR}/utils/utils/parse.h
   ${PROJECT_ROOT_DIR}/utils/utils/properties.h
   ${PROJECT_ROOT_DIR}/utils/utils/t42log.h
   ${PROJECT_ROOT_DIR}/utils/utils/thread.h
   ${PROJECT_ROOT_DIR}/utils/utils/threadMonitor.h
   ${PROJECT_ROOT_DIR}/utils/utils/time.h
   ${PROJECT_ROOT_DIR}/utils/utils/mama/mamaDictionaryWrapper.h
   ${PROJECT_ROOT_DIR}/utils/utils/mama/mamaFieldDescriptorWrapper.h
   ${PROJECT_ROOT_DIR}/utils/utils/mama/types.h
   ${PROJECT_ROOT_DIR}/utils/utils/thread/interlockedInt.h
   ${PROJECT_ROOT_DIR}/utils/utils/thread/lock.h
   ${PROJECT_ROOT_DIR}/utils/utils/thread/semaphore.h

   ${PROJECT_ROOT_DIR}/utils/utils/environment.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/filesystem.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/os.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/parse.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/properties.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/t42log.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/threadMonitor.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/time.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/mama/mamaDictionaryWrapper.cpp
   ${PROJECT_ROOT_DIR}/utils/utils/mama/mamaFieldDescriptorWrapper.cpp

   # Special files
   config.h.in
)

# The Tick42 enhanced module
if (RMDS_ENHANCED)
  message(STATUS "Build Enhanced bridge.")
  include(${PROJECT_ROOT_DIR}/tick42rmds/enhanced/enhanced_modules.txt)
endif (RMDS_ENHANCED)



add_library(mamatick42rmdsimpl SHARED ${SRC_FILES})

#find_library (UTILS_BINARY_LIB
#  NAMES
#     utils
#  PATHS
#     CMAKE_CURRENT_BINARY_DIR/lib
#     CMAKE_CURRENT_BINARY_DIR/utils
#  )

#message ("UTILS_BINARY_LIB=${UTILS_BINARY_LIB}")

set (
   LINK_LIBRARIES_LIST
   ${Boost_FILESYSTEM_LIBRARY}
   ${Boost_DATE_TIME_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${OPENMAMA_LIBRARIES}
   ${OPENMAMA_COMMON_LIBRARIES}
   ${UPA_CPP_LIBRARIES}
#  ${UPA_CPP_LIBRARIES_PATH}
#  ${UTILS_BINARY_LIB}
   )

message ("LINK_LIBRARIES_LIST=${LINK_LIBRARIES_LIST}")
if (MSVC)
   LIST(APPEND LINK_LIBRARIES_LIST wsock32 ) # fixme: move this one to the upa package
endif (MSVC)

target_link_libraries(
   mamatick42rmdsimpl
   ${LINK_LIBRARIES_LIST}
#  utils
   )

install(TARGETS mamatick42rmdsimpl
  # IMPORTANT: Add the mamatick42rmdsimpl library to the "export-set"
  EXPORT RmdsBridgeTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT shlib
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/tick42rmds"
    COMPONENT dev)
