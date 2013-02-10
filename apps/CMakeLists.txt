ADD_SUBDIRECTORY( cubemapviewer )

IF( wxWidgets_FOUND )
    ADD_SUBDIRECTORY( wxapp )
    ADD_SUBDIRECTORY( wxpick )
ENDIF( wxWidgets_FOUND )
