# Required Vars:
# ${LIB_NAME}
# ${LIB_PUBLIC_HEADERS}

SET(INSTALL_INCDIR include)
SET(INSTALL_BINDIR bin)
IF(WIN32)
    SET(INSTALL_LIBDIR bin)
    SET(INSTALL_ARCHIVEDIR lib)
ELSE()
    SET(INSTALL_LIBDIR lib${LIB_POSTFIX})
    SET(INSTALL_ARCHIVEDIR lib${LIB_POSTFIX})
ENDIF()

SET(HEADERS_GROUP "Header Files")

SOURCE_GROUP(
    ${HEADERS_GROUP}
    FILES ${LIB_PUBLIC_HEADERS}
)

INSTALL(
    TARGETS ${LIB_NAME}
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT runtime
    LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT runtime
    ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT development
)

# FIXME: Do not run for OS X framework
INSTALL(
    FILES        ${LIB_PUBLIC_HEADERS}
    DESTINATION ${INSTALL_INCDIR}/${LIB_NAME}
    COMPONENT development
)
