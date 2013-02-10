set( _imageFileList
    fig00-verticalslice.jpg
    fig01-verticalslice.jpg
    fig02-verticalslice.jpg
    fig03-verticalslice.jpg
    fig04-shaderffp.jpg
    fig05-skydome.jpg
    fig06-skydome.jpg
    fig07-backdropfx.jpg
    fig08-glow.jpg
    fig09-glow.jpg
    fig10-glow.jpg
    fig11-glow.jpg
)

foreach( curImage ${_imageFileList} )
    configure_file( ${curImage}
        ${PROJECT_BINARY_DIR}/doc/html/${curImage} COPYONLY )
endforeach()
