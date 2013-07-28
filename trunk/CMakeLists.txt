CMAKE_MINIMUM_REQUIRED( VERSION 2.8.8 )
PROJECT( backdropFX )

SET( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}" )

SET(BACKDROPFX_MAJOR_VERSION 0)
SET(BACKDROPFX_MINOR_VERSION 3)
SET(BACKDROPFX_SUB_VERSION 0)
SET(BACKDROPFX_VERSION ${BACKDROPFX_MAJOR_VERSION}.${BACKDROPFX_MINOR_VERSION}.${BACKDROPFX_SUB_VERSION})
set(VES_PACKAGE_VERSION ${BACKDROPFX_VERSION})

include(VESBinAndLibDirs)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${VES_INSTALL_ARCHIVEDIR} )
if( WIN32 )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
else()
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
endif()

IF( WIN32 AND MSVC )
    OPTION( WIN32_USE_MP "Build with multiple processes (/MP option; VS 2005 and above)." ON )
    MARK_AS_ADVANCED( WIN32_USE_MP )
    IF( WIN32_USE_MP )
        SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" )
    ENDIF( WIN32_USE_MP )
ENDIF( WIN32 AND MSVC )

INCLUDE( CMakeMacros )

include( FindOSGHelper )

FIND_PACKAGE( OSGEphemeris REQUIRED )
FIND_PACKAGE( osgWorks 2.0.0 REQUIRED )
if( osgWorks_FOUND )
    include( ${osgWorks_USE_FILE} )
endif()
FIND_PACKAGE( OSGWxTree )
FIND_PACKAGE( wxWidgets COMPONENTS base core gl )
FIND_PACKAGE( OpenGL REQUIRED )
set( BOOST_ROOT "Please specify" CACHE PATH "Root directory where Boost is installed" )
FIND_PACKAGE( Boost 1.36.0 REQUIRED )
FIND_PACKAGE( PNG REQUIRED )

# Removed per redmine issue 384.
# include( FindBulletHelper )
# FIND_PACKAGE( OSGBulletPlus )
# FIND_PACKAGE( OSGBullet )

# Add project-wide include & lib paths
INCLUDE_DIRECTORIES( 
    ${PROJECT_SOURCE_DIR}/include
    ${OSGEPHEMERIS_INCLUDE_DIR}
    ${OSG_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIR}
)
#IF( OSGBULLETPLUS_FOUND )
#    INCLUDE_DIRECTORIES( ${OSGBULLETPLUS_INCLUDE_DIR} )
#ENDIF( OSGBULLETPLUS_FOUND )
IF( OSGBULLET_FOUND )
    INCLUDE_DIRECTORIES( ${OSGBULLET_INCLUDE_DIR} )
ENDIF( OSGBULLET_FOUND )
IF( BULLET_FOUND )
    OPTION( BDFX_USE_DOUBLE_PRECISION "Use Bullet built with double precision" OFF )
    IF( BDFX_USE_DOUBLE_PRECISION )
        ADD_DEFINITIONS( -DBT_USE_DOUBLE_PRECISION)
    ENDIF( BDFX_USE_DOUBLE_PRECISION )

    INCLUDE_DIRECTORIES(
        ${BULLET_INCLUDE_DIRS}
    )
ENDIF( BULLET_FOUND )
IF( OSGWXTREE_FOUND )
    INCLUDE_DIRECTORIES(
        ${OSGWXTREE_INCLUDE_DIR}
        ${wxWidgets_INCLUDE_DIRS}
    )
ENDIF( OSGWXTREE_FOUND )

OPTION( BDFX_PROFILE_ENABLE "Select to enable profiling code and profiling console output." OFF )
IF( BDFX_PROFILE_ENABLE )
    ADD_DEFINITIONS( -D__BDFX_PROFILE_ENABLE )
ENDIF( BDFX_PROFILE_ENABLE )


OPTION( BDFX_BUILD_APPS "Enable to build applications" ON )
IF( BDFX_BUILD_APPS )
    ADD_SUBDIRECTORY( apps )
ENDIF( BDFX_BUILD_APPS )

OPTION( BDFX_BUILD_PROTOS "Enable to build prototypes" ON )
IF( BDFX_BUILD_PROTOS )
    ADD_SUBDIRECTORY( protos )
ENDIF( BDFX_BUILD_PROTOS )

OPTION( BDFX_BUILD_TESTS "Enable to build Tests" ON )
IF( BDFX_BUILD_TESTS )
    ADD_SUBDIRECTORY( tests )
ENDIF( BDFX_BUILD_TESTS )

OPTION( BDFX_BUILD_EXAMPLES "Enable to build Examples" ON )
IF( BDFX_BUILD_EXAMPLES )
    ADD_SUBDIRECTORY( examples )
ENDIF( BDFX_BUILD_EXAMPLES )

option( BDFX_INSTALL_DATA "Enable to add the data directory to the install target" ON )
if( BDFX_INSTALL_DATA )
    set( dataInstDir "share/${CMAKE_PROJECT_NAME}" )
    set( dataRelDir "data" )
      INSTALL_DATA( ${dataRelDir} ${dataInstDir}/${dataRelDir} "" )
    set( dataRelDir "data/shaders" )
      INSTALL_DATA( ${dataRelDir} ${dataInstDir}/${dataRelDir} "" )
    set( dataRelDir "data/shaders/effects" )
      INSTALL_DATA( ${dataRelDir} ${dataInstDir}/${dataRelDir} "" )
    set( dataRelDir "data/shaders/gl2" )
      INSTALL_DATA( ${dataRelDir} ${dataInstDir}/${dataRelDir} "" )
endif()

ADD_SUBDIRECTORY( src/backdropFX )



#
# Doxygen
option( BACKDROPFX_DOCUMENTATION "Enable to create build target for Doxygen documentation." OFF )
mark_as_advanced( CLEAR BACKDROPFX_DOCUMENTATION )
if( BACKDROPFX_DOCUMENTATION )
    FIND_PACKAGE( Doxygen REQUIRED )
    SET( HAVE_DOT "NO" )
    IF( DOXYGEN_DOT_PATH )
        SET( HAVE_DOT "YES" )
    ENDIF()
    
    CONFIGURE_FILE( ${PROJECT_SOURCE_DIR}/doc/doxyfile.cmake
        ${PROJECT_BINARY_DIR}/doc/doxyfile
    )
    ADD_CUSTOM_TARGET( Documentation ${DOXYGEN_EXECUTABLE}
        ${PROJECT_BINARY_DIR}/doc/doxyfile
    )

    add_subdirectory( doc/images )
endif()

if(MSVC)
    install(
        DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/\${CMAKE_INSTALL_CONFIG_NAME}/
        DESTINATION ${VES_INSTALL_LIBDIR}
        USE_SOURCE_PERMISSIONS
        COMPONENT libosgworks-dev
        FILES_MATCHING PATTERN "*.pdb"
    )
endif(MSVC)

set( VES_EXPORT_LIBRARY_TARGETS backdropFX )

include(VESInstallAutoDiscoveryTools)

#
#Setup and install the fpc file

if(WIN32)
    set(BACKDROPFX_FPC_LIBRARIES "/libpath:\"\${libdir}\" backdropFX.lib")
    set(BACKDROPFX_FPC_INCLUDES "/I\"\${includedir}\"")
else(WIN32)
    set(BACKDROPFX_FPC_INCLUDES "-I\${includedir}")
    if(APPLE)
        set(BACKDROPFX_FPC_LIBRARIES "-L\${libdir} -lbackdropFX")
    else(APPLE)
        set(BACKDROPFX_FPC_LIBRARIES "-L\${libdir} -lbackdropFX")
    endif(APPLE)
endif(WIN32)

configure_file(data/backdropfx.fpc.in
  "${PROJECT_BINARY_DIR}/lib/flagpoll/backdropfx.fpc" @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/flagpoll/backdropfx.fpc
        DESTINATION lib/flagpoll )

