# Locate osgEphemeris.
#
# This script defines:
#   OSGEPHEMERIS_FOUND, set to 1 if found
#   OSGEPHEMERIS_LIBRARIES
#   OSGEPHEMERIS_INCLUDE_DIR
#
# This script will look in standard locations for installed osgEphemeris. However, if you
# install osgEphemeris into a non-standard location, you can use the OSGEPHEMERIS_ROOT
# variable (in environment or CMake) to specify the location.
#
# You can also use osgEphemeris out of a source tree by specifying OSGEPHEMERIS_SOURCE_DIR
# and OSGEPHEMERIS_BUILD_DIR (in environment or CMake).


SET( OSGEPHEMERIS_BUILD_DIR "" CACHE PATH "If using osgEphemeris out of a source tree, specify the build directory." )
SET( OSGEPHEMERIS_SOURCE_DIR "" CACHE PATH "If using osgEphemeris out of a source tree, specify the root of the source tree." )
SET( OSGEPHEMERIS_ROOT "" CACHE PATH "Specify non-standard osgEphemeris install directory. It is the parent of the include and lib dirs." )

MACRO( FIND_OSGEPHEMERIS_INCLUDE THIS_OSGEPHEMERIS_INCLUDE_DIR THIS_OSGEPHEMERIS_INCLUDE_FILE )
    UNSET( ${THIS_OSGEPHEMERIS_INCLUDE_DIR} )
    MARK_AS_ADVANCED( ${THIS_OSGEPHEMERIS_INCLUDE_DIR} )
    FIND_PATH( ${THIS_OSGEPHEMERIS_INCLUDE_DIR} ${THIS_OSGEPHEMERIS_INCLUDE_FILE}
        PATHS
            ${OSGEPHEMERIS_ROOT}
            $ENV{OSGEPHEMERIS_ROOT}
            ${OSGEPHEMERIS_SOURCE_DIR}
            $ENV{OSGEPHEMERIS_SOURCE_DIR}
            /usr/local
            /usr
            /sw/ # Fink
            /opt/local # DarwinPorts
            /opt/csw # Blastwave
            /opt
            "C:/Program Files/osgEphemeris"
            "C:/Program Files (x86)/osgEphemeris"
            ~/Library/Frameworks
            /Library/Frameworks
        PATH_SUFFIXES
            include
            .
    )
ENDMACRO( FIND_OSGEPHEMERIS_INCLUDE THIS_OSGEPHEMERIS_INCLUDE_DIR THIS_OSGEPHEMERIS_INCLUDE_FILE )

FIND_OSGEPHEMERIS_INCLUDE( OSGEPHEMERIS_INCLUDE_DIR osgEphemeris/EphemerisModel.h )
# message( STATUS ${OSGEPHEMERIS_INCLUDE_DIR} )

MACRO( FIND_OSGEPHEMERIS_LIBRARY MYLIBRARY MYLIBRARYNAME )
    UNSET( ${MYLIBRARY} CACHE )
    UNSET( ${MYLIBRARY}_debug CACHE )
    MARK_AS_ADVANCED( ${MYLIBRARY} )
    MARK_AS_ADVANCED( ${MYLIBRARY}_debug )
    FIND_LIBRARY( ${MYLIBRARY}
        NAMES
            ${MYLIBRARYNAME}
        PATHS
            ${OSGEPHEMERIS_ROOT}
            $ENV{OSGEPHEMERIS_ROOT}
            ${OSGEPHEMERIS_BUILD_DIR}
            $ENV{OSGEPHEMERIS_BUILD_DIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            "C:/Program Files/osgEphemeris"
            "C:/Program Files (x86)/osgEphemeris"
            /usr/freeware/lib64
        PATH_SUFFIXES
            lib
            lib/Release
            .
    )
    FIND_LIBRARY( ${MYLIBRARY}_debug
        NAMES
            ${MYLIBRARYNAME}d
        PATHS
            ${OSGEPHEMERIS_ROOT}
            $ENV{OSGEPHEMERIS_ROOT}
            ${OSGEPHEMERIS_BUILD_DIR}
            $ENV{OSGEPHEMERIS_BUILD_DIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            "C:/Program Files/osgEphemeris"
            "C:/Program Files (x86)/osgEphemeris"
            /usr/freeware/lib64
        PATH_SUFFIXES
            lib
            lib/Debug
            .
    )
#    message( STATUS ${${MYLIBRARY}} ${${MYLIBRARY}_debug} )
#    message( STATUS ${MYLIBRARYNAME} )
    IF( ${MYLIBRARY} )
        SET( OSGEPHEMERIS_LIBRARIES ${OSGEPHEMERIS_LIBRARIES}
            "optimized" ${${MYLIBRARY}}
        )
    ENDIF( ${MYLIBRARY} )
    IF( ${MYLIBRARY}_debug )
        SET( OSGEPHEMERIS_LIBRARIES ${OSGEPHEMERIS_LIBRARIES}
            "debug" ${${MYLIBRARY}_debug}
        )
    ENDIF( ${MYLIBRARY}_debug )
ENDMACRO(FIND_OSGEPHEMERIS_LIBRARY LIBRARY LIBRARYNAME)

FIND_OSGEPHEMERIS_LIBRARY( OSGEPHEMERIS_LIBRARY osgEphemeris )

SET( OSGEPHEMERIS_FOUND 0 )
IF( OSGEPHEMERIS_LIBRARIES AND OSGEPHEMERIS_INCLUDE_DIR )
    SET( OSGEPHEMERIS_FOUND 1 )
ENDIF( OSGEPHEMERIS_LIBRARIES AND OSGEPHEMERIS_INCLUDE_DIR )
