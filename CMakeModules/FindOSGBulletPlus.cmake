# Locate osgBulletPlus.
#
# This script defines:
#   OSGBULLETPLUS_FOUND, set to 1 if found
#   OSGBULLETPLUS_LIBRARIES
#   OSGBULLETPLUS_INCLUDE_DIR
#
# This script will look in standard locations for installed osgBulletPlus. However, if you
# install osgBulletPlus into a non-standard location, you can use the OSGBULLETPLUS_ROOT
# variable (in environment or CMake) to specify the location.
#
# You can also use osgBullet out of a source tree by specifying OSGBULLETPLUS_SOURCE_DIR
# and OSGBULLETPLUS_BUILD_DIR (in environment or CMake).


SET( OSGBULLETPLUS_BUILD_DIR "" CACHE PATH "If using osgBulletPlus out of a source tree, specify the build directory." )
SET( OSGBULLETPLUS_SOURCE_DIR "" CACHE PATH "If using osgBulletPlus out of a source tree, specify the root of the source tree." )
SET( OSGBULLETPLUS_ROOT "" CACHE PATH "Specify non-standard osgBulletPlus install directory. It is the parent of the include and lib dirs." )

MACRO( FIND_OSGBULLETPLUS_INCLUDE THIS_OSGBULLETPLUS_INCLUDE_DIR THIS_OSGBULLETPLUS_INCLUDE_FILE )
    UNSET( ${THIS_OSGBULLETPLUS_INCLUDE_DIR} CACHE )
    MARK_AS_ADVANCED( ${THIS_OSGBULLETPLUS_INCLUDE_DIR} )
    FIND_PATH( ${THIS_OSGBULLETPLUS_INCLUDE_DIR} ${THIS_OSGBULLETPLUS_INCLUDE_FILE}
        PATHS
            ${OSGBULLETPLUS_ROOT}
            $ENV{OSGBULLETPLUS_ROOT}
            ${OSGBULLETPLUS_SOURCE_DIR}
            $ENV{OSGBULLETPLUS_SOURCE_DIR}
            /usr/local
            /usr
            /sw/ # Fink
            /opt/local # DarwinPorts
            /opt/csw # Blastwave
            /opt
            "C:/Program Files/osgBulletPlus"
            "C:/Program Files (x86)/osgBulletPlus"
            ~/Library/Frameworks
            /Library/Frameworks
        PATH_SUFFIXES
            include
            .
    )
ENDMACRO( FIND_OSGBULLETPLUS_INCLUDE THIS_OSGBULLETPLUS_INCLUDE_DIR THIS_OSGBULLETPLUS_INCLUDE_FILE )

FIND_OSGBULLETPLUS_INCLUDE( OSGBULLETPLUS_INCLUDE_DIR osgbBulletPlus/HandNode.h )
# message( STATUS ${OSGBULLETPLUS_INCLUDE_DIR} )

MACRO( FIND_OSGBULLETPLUS_LIBRARY MYLIBRARY MYLIBRARYNAME )
    UNSET( ${MYLIBRARY} CACHE )
    UNSET( ${MYLIBRARY}_debug CACHE )
    MARK_AS_ADVANCED( ${MYLIBRARY} )
    MARK_AS_ADVANCED( ${MYLIBRARY}_debug )
    FIND_LIBRARY( ${MYLIBRARY}
        NAMES
            ${MYLIBRARYNAME}
        PATHS
            ${OSGBULLETPLUS_ROOT}
            $ENV{OSGBULLETPLUS_ROOT}
            ${OSGBULLETPLUS_BUILD_DIR}
            $ENV{OSGBULLETPLUS_BUILD_DIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            "C:/Program Files/osgBulletPlus"
            "C:/Program Files (x86)/osgBulletPlus"
            /usr/freeware/lib64
        PATH_SUFFIXES
            lib
            .
    )
    FIND_LIBRARY( ${MYLIBRARY}_debug
        NAMES
            ${MYLIBRARYNAME}d
        PATHS
            ${OSGBULLETPLUS_ROOT}
            $ENV{OSGBULLETPLUS_ROOT}
            ${OSGBULLETPLUS_BUILD_DIR}
            $ENV{OSGBULLETPLUS_BUILD_DIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            "C:/Program Files/osgBulletPlus"
            "C:/Program Files (x86)/osgBulletPlus"
            /usr/freeware/lib64
        PATH_SUFFIXES
            lib
            .
    )
#    message( STATUS ${${MYLIBRARY}} ${${MYLIBRARY}_debug} )
#    message( STATUS ${MYLIBRARYNAME} )
    IF( ${MYLIBRARY} )
        SET( OSGBULLETPLUS_LIBRARIES ${OSGBULLETPLUS_LIBRARIES}
            "optimized" ${${MYLIBRARY}}
        )
    ENDIF( ${MYLIBRARY} )
    IF( ${MYLIBRARY}_debug )
        SET( OSGBULLETPLUS_LIBRARIES ${OSGBULLETPLUS_LIBRARIES}
            "debug" ${${MYLIBRARY}_debug}
        )
    ENDIF( ${MYLIBRARY}_debug )
ENDMACRO(FIND_OSGBULLETPLUS_LIBRARY LIBRARY LIBRARYNAME)

FIND_OSGBULLETPLUS_LIBRARY( OSGBULLETPLUS_LIBRARY osgbBulletPlus )

SET( OSGBULLETPLUS_FOUND 0 )
IF( OSGBULLETPLUS_LIBRARIES AND OSGBULLETPLUS_INCLUDE_DIR )
    SET( OSGBULLETPLUS_FOUND 1 )
ENDIF( OSGBULLETPLUS_LIBRARIES AND OSGBULLETPLUS_INCLUDE_DIR )
