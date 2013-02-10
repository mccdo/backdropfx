# Locate osgWxTree.
#
# This script defines:
#   OSGWXTREE_FOUND, set to 1 if found
#   OSGWXTREE_LIBRARIES
#   OSGWXTREE_INCLUDE_DIR
#
# This script will look in standard locations for installed osgWxTree. However, if you
# install osgWxTree into a non-standard location, you can use the OSGWXTREE_ROOT
# variable (in environment or CMake) to specify the location.
#
# You can also use osgWxTree out of a source tree by specifying OSGWXTREE_SOURCE_DIR
# and OSGWXTREE_BUILD_DIR (in environment or CMake).


SET( OSGWXTREE_BUILD_DIR "" CACHE PATH "If using osgWxTree out of a source tree, specify the build directory." )
SET( OSGWXTREE_SOURCE_DIR "" CACHE PATH "If using osgWxTree out of a source tree, specify the root of the source tree." )
SET( OSGWXTREE_ROOT "" CACHE PATH "Specify non-standard osgWxTree install directory. It is the parent of the include and lib dirs." )

MACRO( FIND_OSGWXTREE_INCLUDE THIS_OSGWXTREE_INCLUDE_DIR THIS_OSGWXTREE_INCLUDE_FILE )
    UNSET( ${THIS_OSGWXTREE_INCLUDE_DIR} CACHE )
    MARK_AS_ADVANCED( ${THIS_OSGWXTREE_INCLUDE_DIR} )
    FIND_PATH( ${THIS_OSGWXTREE_INCLUDE_DIR} ${THIS_OSGWXTREE_INCLUDE_FILE}
        PATHS
            ${OSGWXTREE_ROOT}
            $ENV{OSGWXTREE_ROOT}
            ${OSGWXTREE_SOURCE_DIR}
            $ENV{OSGWXTREE_SOURCE_DIR}
            /usr/local
            /usr
            /sw/ # Fink
            /opt/local # DarwinPorts
            /opt/csw # Blastwave
            /opt
            "C:/Program Files/osgWxTree"
            "C:/Program Files (x86)/osgWxTree"
            ~/Library/Frameworks
            /Library/Frameworks
        PATH_SUFFIXES
            include
            .
    )
ENDMACRO( FIND_OSGWXTREE_INCLUDE THIS_OSGWXTREE_INCLUDE_DIR THIS_OSGWXTREE_INCLUDE_FILE )

FIND_OSGWXTREE_INCLUDE( OSGWXTREE_INCLUDE_DIR osgWxTree/TreeControl.h )
# message( STATUS ${OSGWXTREE_INCLUDE_DIR} )

MACRO( FIND_OSGWXTREE_LIBRARY MYLIBRARY MYLIBRARYNAME )
    UNSET( ${MYLIBRARY} CACHE )
    UNSET( ${MYLIBRARY}_debug CACHE )
    MARK_AS_ADVANCED( ${MYLIBRARY} )
    MARK_AS_ADVANCED( ${MYLIBRARY}_debug )
    FIND_LIBRARY( ${MYLIBRARY}
        NAMES ${MYLIBRARYNAME}
        PATHS
            ${OSGWXTREE_ROOT}
            $ENV{OSGWXTREE_ROOT}
            ${OSGWXTREE_BUILD_DIR}
            $ENV{OSGWXTREE_BUILD_DIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            "C:/Program Files/osgWxTree"
            "C:/Program Files (x86)/osgWxTree"
            /usr/freeware/lib64
        PATH_SUFFIXES
            lib
            .
    )
    FIND_LIBRARY( ${MYLIBRARY}_debug
        NAMES ${MYLIBRARYNAME}d
        PATHS
            ${OSGWXTREE_ROOT}
            $ENV{OSGWXTREE_ROOT}
            ${OSGWXTREE_BUILD_DIR}
            $ENV{OSGWXTREE_BUILD_DIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt
            "C:/Program Files/osgWxTree"
            "C:/Program Files (x86)/osgWxTree"
            /usr/freeware/lib64
        PATH_SUFFIXES
            lib
            .
    )
#    message( STATUS ${${MYLIBRARY}} ${${MYLIBRARY}_debug} )
#    message( STATUS ${MYLIBRARYNAME} )
    IF( ${MYLIBRARY} )
        SET( OSGWXTREE_LIBRARIES ${OSGWXTREE_LIBRARIES}
            "optimized" ${${MYLIBRARY}}
        )
    ENDIF( ${MYLIBRARY} )
    IF( ${MYLIBRARY}_debug )
        SET( OSGWXTREE_LIBRARIES ${OSGWXTREE_LIBRARIES}
            "debug" ${${MYLIBRARY}_debug}
        )
    ENDIF( ${MYLIBRARY}_debug )
ENDMACRO(FIND_OSGWXTREE_LIBRARY LIBRARY LIBRARYNAME)

FIND_OSGWXTREE_LIBRARY( OSGWXTREE_LIBRARY osgWxTree )

SET( OSGWXTREE_FOUND 0 )
IF( OSGWXTREE_LIBRARIES AND OSGWXTREE_INCLUDE_DIR )
    SET( OSGWXTREE_FOUND 1 )
ENDIF( OSGWXTREE_LIBRARIES AND OSGWXTREE_INCLUDE_DIR )
