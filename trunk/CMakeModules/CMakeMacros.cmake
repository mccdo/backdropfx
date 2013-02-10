IF(WIN32)
    SET(CMAKE_DEBUG_POSTFIX d)
ENDIF(WIN32)


MACRO( ADD_SHARED_LIBRARY_INTERNAL TRGTNAME )
    ADD_LIBRARY( ${TRGTNAME} SHARED ${ARGN} )
    IF( WIN32 )
        SET_TARGET_PROPERTIES( ${TRGTNAME} PROPERTIES DEBUG_POSTFIX d )
    ENDIF( WIN32 )
    TARGET_LINK_LIBRARIES( ${TRGTNAME}
        ${OSGEPHEMERIS_LIBRARIES}
        ${OSGWORKS_LIBRARIES}
        ${OSG_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${PNG_LIBRARIES}
    )
    SET_TARGET_PROPERTIES( ${TRGTNAME} PROPERTIES PROJECT_LABEL "Lib ${TRGTNAME}" )
ENDMACRO( ADD_SHARED_LIBRARY_INTERNAL TRGTNAME )

MACRO( ADD_OSGPLUGIN TRGTNAME )
    ADD_LIBRARY( ${TRGTNAME} MODULE ${ARGN} )
    if( WIN32 )
        SET_TARGET_PROPERTIES( ${TRGTNAME} PROPERTIES DEBUG_POSTFIX d )
    endif()

    link_internal( ${TRGTNAME}
        backdropFX
    )

    TARGET_LINK_LIBRARIES( ${TRGTNAME}
        ${OSGEPHEMERIS_LIBRARIES}
        ${OSGWORKS_LIBRARIES}
        ${OSG_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${PNG_LIBRARIES}
    )
    SET_TARGET_PROPERTIES( ${TRGTNAME} PROPERTIES PREFIX "" )
    SET_TARGET_PROPERTIES( ${TRGTNAME} PROPERTIES PROJECT_LABEL "Plugin ${TRGTNAME}" )
ENDMACRO( ADD_OSGPLUGIN TRGTNAME )


MACRO( MAKE_EXECUTABLE EXENAME )
#    message( STATUS "making executable ${CATEGORY} ${EXENAME}" )
    ADD_EXECUTABLE_INTERNAL( ${EXENAME}
        ${ARGN}
    )

    link_internal( ${EXENAME}
        backdropFX
    )

    TARGET_LINK_LIBRARIES( ${EXENAME}
        ${OSGEPHEMERIS_LIBRARIES}
#        ${OSGBULLETPLUS_LIBRARIES}
#        ${OSGBULLET_LIBRARIES}
#        ${BULLET_LIBRARIES}
        ${OSGWORKS_LIBRARIES}
        ${OSG_LIBRARIES}
        ${OPENGL_LIBRARIES}
    )

    # Requires ${CATAGORY}
    if( CATEGORY STREQUAL "App" )
        install(
            TARGETS ${EXENAME}
            RUNTIME DESTINATION bin COMPONENT libosgworks
        )
    else()
        install(
            TARGETS ${EXENAME}
            RUNTIME DESTINATION share/${CMAKE_PROJECT_NAME}/bin COMPONENT libosgworks
        )
    endif()
    SET_TARGET_PROPERTIES( ${EXENAME} PROPERTIES PROJECT_LABEL "${CATEGORY} ${EXENAME}" )
ENDMACRO( MAKE_EXECUTABLE EXENAME CATEGORY )

MACRO( MAKE_WX_EXECUTABLE EXENAME )
#    message( STATUS "making wx executable ${CATEGORY} ${EXENAME}" )
    ADD_DEFINITIONS( ${wxWidgets_DEFINITIONS} )
    ADD_EXECUTABLE_INTERNAL( ${EXENAME}
        WIN32
        ${ARGN}
    )

    link_internal( ${EXENAME}
        backdropFX
    )

    TARGET_LINK_LIBRARIES( ${EXENAME}
        ${OSGEPHEMERIS_LIBRARIES}
#        ${OSGBULLETPLUS_LIBRARIES}
#        ${OSGBULLET_LIBRARIES}
#        ${BULLET_LIBRARIES}
        ${OSGWXTREE_LIBRARIES}
        ${OSGWORKS_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${OSG_LIBRARIES}
        ${OPENGL_LIBRARIES}
    )

    # Requires ${CATAGORY}
    IF( CATEGORY STREQUAL "App" )
        INSTALL(
            TARGETS ${EXENAME}
            RUNTIME DESTINATION bin COMPONENT libosgworks
        )
    ENDIF( CATEGORY STREQUAL "App" )
    SET_TARGET_PROPERTIES( ${EXENAME} PROPERTIES PROJECT_LABEL "${CATEGORY} ${EXENAME}" )
ENDMACRO( MAKE_WX_EXECUTABLE EXENAME CATEGORY )

MACRO( ADD_EXECUTABLE_INTERNAL TRGTNAME )
    ADD_EXECUTABLE( ${TRGTNAME} ${ARGN} )
    IF( WIN32 )
        SET_TARGET_PROPERTIES( ${TRGTNAME} PROPERTIES DEBUG_POSTFIX d )
    ENDIF(WIN32)
ENDMACRO( ADD_EXECUTABLE_INTERNAL TRGTNAME )

macro( link_internal TRGTNAME )
    foreach( LINKLIB ${ARGN} )
        TARGET_LINK_LIBRARIES( ${TRGTNAME} optimized "${LINKLIB}" debug "${LINKLIB}" )
    endforeach()
endmacro()

macro( INSTALL_DATA _relSrcDir _destDir _exclude )
    #   _relSrcDir - Relative directory of files to install.
    #   _destDir - Relative destincation install directory.
    #   _exclude - List of files that should not be installed.
    
    # Init. This persists from one invocation to the next, and all we do is
    # append to it, so it must be cleared.
    set( installList "" )

    # Get the raw list of files in _srcDir.
    file( GLOB fileList RELATIVE ${PROJECT_SOURCE_DIR}/${_relSrcDir} ${_relSrcDir}/* )

    foreach( trgtFile ${fileList} )
        # If a file is both not a directory and not on the _exclude list,
        # prepend the relative source dir and place it on the installList.
        list( FIND _exclude ${trgtFile} excludeFile )
        if(    ( ${excludeFile} EQUAL -1 ) AND
               NOT IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${_relSrcDir}/${trgtFile}   )
            list( APPEND installList ${_relSrcDir}/${trgtFile} )
        endif()
    endforeach()

    # Debug: display the final list of files to install.
    #message( STATUS "--- ${installList}" )

    install( FILES ${installList}
        DESTINATION ${_destDir}
        COMPONENT libosgworks
    )
endmacro()
