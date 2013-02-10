SET( CATEGORY App )

# For dev of cube map cloud textures.
# More of a tool, really.
#ADD_SUBDIRECTORY( cubemapviewer )

IF( wxWidgets_FOUND )
    # For turn-on of Wx support
    #ADD_SUBDIRECTORY( wxapp )

    ADD_SUBDIRECTORY( wxpick )

ENDIF( wxWidgets_FOUND )
