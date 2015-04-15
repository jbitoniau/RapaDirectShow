CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

PROJECT( "RapaDirectShow" )

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake" )

IF( CMAKE_SYSTEM_NAME MATCHES "Windows" )
	
	IF( MSVC )
		INCLUDE(RapaConfigureVisualStudio)
	ENDIF()

	INCLUDE( RapaFindDirectShow )
	IF( DIRECTSHOW_FOUND )
		
		INCLUDE_DIRECTORIES( ${DirectShow_INCLUDE_DIR} )
		INCLUDE_DIRECTORIES( include )
		
		SET	(	HEADERS
				include/qedit.h
				include/RDShowCOMObjectSharedPtr.h
				include/RDShowCriticalSectionEnterer.h
				include/RDShowUnicode.h
				include/RDShowMemoryBuffer.h
				include/RDShowImageFormat.h
				include/RDShowImage.h
				include/RDShowImageConverter.h
				include/RDShowCaptureSettings.h
				include/RDShowCapturedImage.h
				include/RDShowDeviceInternals.h
				include/RDShowDevice.h
				include/RDShowDeviceManager.h
			)	
		
		SET	(	SOURCES
				src/RDShowCOMObjectSharedPtr.cpp
				src/RDShowCriticalSectionEnterer.cpp		
				src/RDShowUnicode.cpp
				src/RDShowMemoryBuffer.cpp
				src/RDShowImageFormat.cpp
				src/RDShowImage.cpp
				src/RDShowImageConverter.cpp
				src/RDShowCaptureSettings.cpp
				src/RDShowCapturedImage.cpp
				src/RDShowDeviceInternals.cpp
				src/RDShowDevice.cpp
				src/RDShowDeviceManager.cpp		
			)	
	
		SOURCE_GROUP("" FILES ${HEADERS} ${SOURCES} )		# Avoid "Header Files" and "Source Files" virtual folders in VisualStudio

		SET(CMAKE_DEBUG_POSTFIX "d")
		ADD_LIBRARY( ${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES} )
		TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${DirectShow_LIBRARY} ) 
		
		#
		# Install
		#
		INSTALL(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
				LIBRARY DESTINATION lib
				ARCHIVE DESTINATION lib
				RUNTIME DESTINATION bin )
				#INCLUDES DESTINATION include )		# If uncommented, the ${PROJECT_NAME} target contains INCLUDE_DIRECTORIES information. Importing the target automatically adds this directory to the INCLUDE_DIRECTORIES.
		SET( TARGET_NAMESPACE Rapa:: )
		INSTALL( FILES ${HEADERS} DESTINATION include COMPONENT Devel )		
		EXPORT( EXPORT ${PROJECT_NAME}Targets FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Targets.cmake" NAMESPACE ${TARGET_NAMESPACE} )
		CONFIGURE_FILE( cmake/${PROJECT_NAME}Config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake" @ONLY )
		SET( ConfigPackageLocation lib/cmake/${PROJECT_NAME} )
		INSTALL(EXPORT ${PROJECT_NAME}Targets
				FILE ${PROJECT_NAME}Targets.cmake
				NAMESPACE ${TARGET_NAMESPACE}
				DESTINATION ${ConfigPackageLocation} )
		INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake" DESTINATION ${ConfigPackageLocation} COMPONENT Devel )
	
		ADD_SUBDIRECTORY( samples )
		
	ELSE()
		MESSAGE("DirectShow not found")
	ENDIF()
ELSE()
	MESSAGE("${PROJECT_NAME} is Windows only")
ENDIF()







 
 